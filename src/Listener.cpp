/// @file       Listener.cpp
/// @brief      UDP receiver thread
/// @details    Receives flight data from UDP messages and stores the data
///             in a map of lists of flight data objects
/// @author     Birger Hoppe
/// @copyright  (c) 2022 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#include "XPPlanes.h"

#if APL == 1 || LIN == 1
#include <unistd.h>                     // for pipe
#include <sys/fcntl.h>                  // for F_SETFL, O_NONBLOCK
#endif

//
// MARK: RECEIVING Remote Data (Worker Thread)
//

#define BCST_LOCALHOST      "0.0.0.0"
#define INFO_LISTEN_BEGIN   "Receiver started listening to %s:%d"
#define INFO_LISTEN_RCVD    "Receiver received data from %.*s on %s, will start message processing"
#define ERR_LISTEN_THREAD   "Exception in listener: %s"

// module-global variables
constexpr int LISTEN_INTVL = 15;                ///< listen for this many seconds before thread wakes up again
static std::thread gThrMC;                      ///< remote listening/sending thread
static XPMP2::UDPMulticast* gpMc = nullptr;     ///< multicast socket (destructor uses locks, which don't work during module shutdown, so can't create a global object due to its exit-time destructor)
static XPMP2::UDPReceiver* gpUDP = nullptr;     ///< UDP receiver socket (destructor uses locks, which don't work during module shutdown, so can't create a global object due to its exit-time destructor)

#if APL == 1 || LIN == 1
/// the self-pipe to shut down the APRS thread gracefully
static XPMP2::SOCKET gSelfPipe[2] = { XPMP2::INVALID_SOCKET, XPMP2::INVALID_SOCKET };
#endif

/// Conditions for continued receive operation
bool ListenContinue ()
{
    return
    glob.eStatus > GlobVars::STATUS_INACTIVE &&     // status not inactive
    ((gpMc && gpMc->isOpen()) ||                    // multicast listening or
     (gpUDP && gpUDP->isOpen()));                   // broadcast listening
}


/// Thread main function for the receiver
void ListenMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME(XPPLANES "_Listen");
    
    try {
        LOG_ASSERT(gpMc != nullptr);
        LOG_ASSERT(gpUDP != nullptr);
        
        // Set global status to: we are "waiting" for data
        glob.eStatus = GlobVars::STATUS_WAITING;

        // Create a multicast socket, if so configured
        int maxSock = 0;
        if (glob.listenMCPort > 0) {
            gpMc->Join(glob.listenMCGroup, glob.listenMCPort, glob.remoteTTL,
                       size_t(glob.remoteBufSize));
            maxSock = std::max(maxSock, (int)gpMc->getSocket() + 1);
        }
        
        // Create a UDP broadcast socket, if so configured
        if (glob.listenBcstPort > 0) {
            gpUDP->Open (BCST_LOCALHOST, glob.listenBcstPort, size_t(glob.remoteBufSize));
            maxSock = std::max(maxSock, (int)gpUDP->getSocket() + 1);
        }
        
#if APL == 1 || LIN == 1
        // the self-pipe to shut down the TCP socket gracefully
        if (pipe(gSelfPipe) < 0)
            throw XPMP2::NetRuntimeError("Couldn't create self-pipe");
        fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
        maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
                
        // Log message on what's open and listening
        for (const XPMP2::SocketNetworking* pNet: { (XPMP2::SocketNetworking*)gpMc, (XPMP2::SocketNetworking*)gpUDP }) {
            if (pNet->isOpen()) {
                LOG_MSG(logMSG, INFO_LISTEN_BEGIN, pNet->getAddr().c_str(), pNet->getPort());
            }
        }
        
        // *** Main listening loop ***
        while (ListenContinue())
        {
            // wait for some data on either socket (multicast or self-pipe)
            fd_set sRead;
            FD_ZERO(&sRead);
            if (gpMc->isOpen())
                FD_SET(gpMc->getSocket(), &sRead);      // wait for multicast
            if (gpUDP->isOpen())
                FD_SET(gpUDP->getSocket(), &sRead);     // wait for broadcast
#if APL == 1 || LIN == 1
            FD_SET(gSelfPipe[0], &sRead);               // check the self-pipe
#endif
            // Timeout is 15s, just to make sure that every once in a while we wake up here
            struct timeval timeout = { LISTEN_INTVL, 0 };
            int retval = select(maxSock, &sRead, NULL, NULL, &timeout);

            // short-cut if we are to shut down (return from 'select' due to closed socket)
            if (!ListenContinue())
                break;

            // select call failed???
            if (retval == -1)
                throw XPMP2::NetRuntimeError("'select' failed");
            
            // select successful - there is multicast and/or broadcast data!
            else if (retval > 0)
            {
                // loop over both multicast and broadcast sockets
                for (XPMP2::SocketNetworking* pNet: { (XPMP2::SocketNetworking*)gpMc, (XPMP2::SocketNetworking*)gpUDP })
                {
                    // No message waiting? -> skip
                    if (!FD_ISSET(pNet->getSocket(), &sRead))
                        continue;
                    
                    // Receive the data
                    const size_t recvSize = pNet->recv();
                    if (recvSize < 10) {
                        LOG_MSG(logWARN, "Received too small message with just %lu bytes: %s", (unsigned long)recvSize, pNet->getBuf());
                        continue;
                    }
                    
                    // buffer to network data
                    const char* theData = pNet->getBuf();
                    
                    // Convert the data into a FlightData object, that can fail and would raise an exception
                    ptrFlightDataTy pFD;
                    try {
                        pFD = std::make_shared<FlightData>(theData);
                        // insertion into the map/list of flight data is protected by a mutex
                        std::lock_guard<std::mutex> guard(glob.mtxListFD);
                        listFlightDataTy& listFD = glob.mapListFD[pFD->_modeS_id];
                        listFD.emplace_back(std::move(pFD));
                    }
                    catch (const FlightData_error& e) {
                        LOG_MSG(logDEBUG, "Couldn't convert to FlightData object, unknown format or data insufficient:\n%.80s", theData);
                    }
                    catch (const std::exception& e) {
                        LOG_MSG(logWARN, "Couldn't convert to FlightData object, %s:\n%.80s", e.what(), theData);
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_MSG(logERR, ERR_LISTEN_THREAD, e.what());
    }
    
    // close the sockets
    gpMc->Close();
    gpUDP->Close();

#if APL == 1 || LIN == 1
    // close the self-pipe sockets
    for (XPMP2::SOCKET &s: gSelfPipe) {
        if (s != XPMP2::INVALID_SOCKET) close(s);
        s = XPMP2::INVALID_SOCKET;
    }
#endif
    
    // make sure the end of the thread is recognized and joined
    glob.eStatus = GlobVars::STATUS_INACTIVE;
}

//
// MARK: Global public functions (XP Main Thread)
//

// Initialize the module and start the network listener thread, returns success
bool ListenStartup ()
{
    // At least one port needs to be configured
    if (glob.listenMCPort <= 0 && glob.listenBcstPort <= 0) {
        LOG_MSG(logFATAL, "Both multicast and broadcast ports are configured off, cannot listen to anything; change config!");
        return false;
    }
    
    // Create the global multicast and UDP objects
    if (!gpMc)
        gpMc = new XPMP2::UDPMulticast();
    LOG_ASSERT(gpMc);
    if (!gpUDP)
        gpUDP = new XPMP2::UDPReceiver();
    LOG_ASSERT(gpUDP);
    
    // Start the background thread to listen to multicast
    // Can only do that if currently off
    if (glob.eStatus != GlobVars::STATUS_INACTIVE)
        return false;
    
    // Is or was there a thread running?
    if (gThrMC.joinable()) {
        gThrMC.join();                  // wait for the current thread to exit (which it should because glob.eStatus == STATUS_INACTIVE)
        gThrMC = std::thread();
    }
    
    // Start the thread
    gThrMC = std::thread(ListenMain);
    return gThrMC.joinable();
}

// Stop the network thread, wait for its shutdown, and cleanup the module
void ListenShutdown ()
{
    // Stop all threads and communication with the network
    if (gThrMC.joinable()) {
        // indicate: shutdown!
        glob.eStatus = GlobVars::STATUS_INACTIVE;
#if APL == 1 || LIN == 1
        // Mac/Lin: Try writing something to the self-pipe to stop gracefully
        if (gSelfPipe[1] == XPMP2::INVALID_SOCKET ||
            write(gSelfPipe[1], "STOP", 4) < 0)
            // if the self-pipe didn't work:
#endif
        {
            if (gpMc)
                gpMc->Close();
            if (gpUDP)
                gpUDP->Close();
        }

        // wait for the network thread to finish
        gThrMC.join();
        gThrMC = std::thread();
    }

    // remove the networking objects
    if (gpMc) {
        delete gpMc;
        gpMc = nullptr;
    }
    if (gpUDP) {
        delete gpUDP;
        gpUDP = nullptr;
    }
}
