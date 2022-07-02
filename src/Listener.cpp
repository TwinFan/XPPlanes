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

#define DEBUG_MC_RECV_BEGIN "Receiver started listening to %s:%d"
#define INFO_MC_RECV_BEGIN  "Receiver started listening on the network..."
#define INFO_MC_RECV_RCVD   "Receiver received data from %.*s on %s, will start message processing"
#define ERR_MC_THREAD       "Exception in multicast handling: %s"

// module-global variables
constexpr int LISTEN_INTVL = 15;                ///< listen for this many seconds before thread wakes up again
static std::thread gThrMC;                      ///< remote listening/sending thread
static XPMP2::UDPMulticast* gpMc = nullptr;     ///< multicast socket for listening/sending (destructor uses locks, which don't work during module shutdown, so can't create a global object due to its exit-time destructor)

#if APL == 1 || LIN == 1
/// the self-pipe to shut down the APRS thread gracefully
static XPMP2::SOCKET gSelfPipe[2] = { XPMP2::INVALID_SOCKET, XPMP2::INVALID_SOCKET };
#endif

/// Conditions for continued receive operation
inline bool ListenContinue ()
{ return glob.eStatus > GlobVars::STATUS_INACTIVE && gpMc && gpMc->isOpen(); }


/// Thread main function for the receiver
void ListenMain()
{
    // This is a thread main function, set thread's name
    SET_THREAD_NAME(XPPLANES "_Listen");
    
    try {
        LOG_ASSERT(gpMc != nullptr);
        
        // Set global status to: we are "waiting" for data
        glob.eStatus = GlobVars::STATUS_WAITING;

        // Create a multicast socket
        gpMc->Join(glob.remoteMCGroup, glob.remotePort, glob.remoteTTL,
                   size_t(glob.remoteBufSize));
        int maxSock = (int)gpMc->getSocket() + 1;
#if APL == 1 || LIN == 1
        // the self-pipe to shut down the TCP socket gracefully
        if (pipe(gSelfPipe) < 0)
            throw XPMP2::NetRuntimeError("Couldn't create self-pipe");
        fcntl(gSelfPipe[0], F_SETFL, O_NONBLOCK);
        maxSock = std::max(maxSock, gSelfPipe[0]+1);
#endif
                
        // *** Main listening loop ***
        
        if (glob.logLvl == logDEBUG)
            LOG_MSG(logDEBUG, DEBUG_MC_RECV_BEGIN, glob.remoteMCGroup.c_str(), glob.remotePort)
        else
            LOG_MSG(logINFO, INFO_MC_RECV_BEGIN)
        
        while (ListenContinue())
        {
            // wait for some data on either socket (multicast or self-pipe)
            fd_set sRead;
            FD_ZERO(&sRead);
            FD_SET(gpMc->getSocket(), &sRead);      // check our socket
#if APL == 1 || LIN == 1
            FD_SET(gSelfPipe[0], &sRead);           // check the self-pipe
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
            
            // select successful - there is multicast data!
            else if (retval > 0 && FD_ISSET(gpMc->getSocket(), &sRead))
            {
                // Receive the data (if we are still waiting then we're interested in the sender's address purely for logging purposes)
                sockaddr saFrom;
                const size_t recvSize = gpMc->RecvMC(nullptr, &saFrom);
                if (recvSize >= 10)
                {
                    const XPMP2::InetAddrTy from(&saFrom);           // extract the numerical address
                    const char* theData = gpMc->getBuf();
                    
                    // TODO: Do something with the received data
                    LOG_MSG(logDEBUG, "Received from %s:\n%s",
                            XPMP2::SocketNetworking::GetAddrString(&saFrom).c_str(),
                            theData);
                    
                } else {
                    LOG_MSG(logWARN, "Received too small message with just %lu bytes", (unsigned long)recvSize);
                }
            }
        }
    }
    catch (std::exception& e) {
        LOG_MSG(logERR, ERR_MC_THREAD, e.what());
    }
    
    // close the multicast socket
    gpMc->Close();

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
    // Create the global multicast object
    if (!gpMc)
        gpMc = new XPMP2::UDPMulticast();
    LOG_ASSERT(gpMc);
    
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
            if (gpMc)
                gpMc->Close();

        // wait for the network thread to finish
        gThrMC.join();
        gThrMC = std::thread();
    }

    // remove the multicast object
    if (gpMc) {
        delete gpMc;
        gpMc = nullptr;
    }
}
