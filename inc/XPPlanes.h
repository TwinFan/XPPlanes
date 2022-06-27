/// @file       XPPlanes.h
/// @brief      Header file covering all includes required for compilingXPPlanes, basis for pre-compiled headers
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

#pragma once

#if IBM
// In MINGW winsock2 must be included before windows.h, which is included by the mingw.*.h headers
#include <winsock2.h>
#include <ws2ipdef.h>           // required for sockaddr_in6 (?)
#include <iphlpapi.h>           // for GetAdaptersAddresses
#include <ws2tcpip.h>
#endif

// Standard C
#include <sys/stat.h>
#include <cmath>
#include <cstdarg>
#include <cassert>
#include <cstring>

// Standard C++
#include <string>
#include <list>
#include <map>
#include <array>
#include <vector>
#include <thread>

// On Windows, 'max' and 'min' are defined macros in conflict with C++ library. Let's undefine them!
#if IBM
#include <direct.h>
#undef max
#undef min
#endif

// X-Plane SDK
#include "XPLMUtilities.h"
#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"

// XPMP2 - Public Header Files
#include "XPMPMultiplayer.h"
#include "XPMPAircraft.h"

// XPPlanes
#include "Constants.h"
#include "Utilities.h"

//
// MARK: Global Configurations and variables
//

/// All global config settings and variables are kept in one structure for convenient access and central definition
struct GlobVars {
public:
    // Settings:
    /// Logging level
    logLevelTy      logLvl      = logINFO;
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Clamp all planes to the ground? Default is `false` as clamping is kinda expensive due to Y-Testing.
    bool            bClampAll   = false;
    /// Replace dataRefs in `.obj` files on load? (defaults to OFF!)
    bool            bObjReplDataRefs = false;
    /// Replace textures in `.obj` files on load if needed?
    bool            bObjReplTextures = true;
    
    /// Global map of all created planes
//    mapAcTy         mapAc;
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    /// Maximum distance for drawing labels? [m], defaults to 3nm
    float           maxLabelDist = 5556.0f;
    /// Cut off labels at XP's reported visibility mit?
    bool            bLabelCutOffAtVisibility = true;
    /// Label font scaling factor
    float           labelFontScaling = 1.0f;
    
    /// Do we want to control X-Plane's AI/Multiplayer planes for TCAS?
    bool            bAITcasControl  = true;
    
    /// Do we feed X-Plane's maps with our aircraft positions?
    bool            bMapEnabled     = true;
    /// Do we show labels with the aircraft icons?
    bool            bMapLabels      = true;
    
    /// @brief The multicast group that we use, which is the same X-Plane is using itself for its BEACON
    /// @see <X-Plane>/Instructions/Exchanging Data with X-Plane.rtfd, chapter "DISCOVER X-PLANE BY A BEACON"
    std::string     remoteMCGroup   = "239.255.1.1";
    /// The port we use is _different_ from the port the X-Plane BEACON uses, so we don't get into conflict
    int             remotePort      = 49788;
    /// Time-to-live, or mumber of hops for a multicast message
    int             remoteTTL       = 8;
    /// Buffer size, ie. max message length we send over multicast
    size_t          remoteBufSize   = 8192;

    /// This plugin's id
    XPLMPluginID    pluginId        = 0;
    /// id of X-Plane's thread (when it is OK to use XP API calls)
    std::thread::id xpThread;
    /// Current X-Xplane's time
    float           now             = NAN;
    /// plugin's menu handle
    XPLMMenuID      hMenu           = NULL;
    
    /// Plugin status
    enum StatusTy {
        STATUS_INACTIVE = 0,        ///< plugin inactive or starting up
        STATUS_WAITING,             ///< waiting for first network messages
        STATUS_ACTIVE,              ///< receiving data, displaying planes
    } eStatus = STATUS_INACTIVE;

public:
    /// Constructor
    GlobVars (logLevelTy _logLvl = logINFO, bool _logMdlMatch = false) :
    logLvl(_logLvl), bLogMdlMatch(_logMdlMatch) {}
    /// Read from a generic `XPMP2.prf` or `XPMP2.<logAcronym>.prf` config file
    void ReadConfigFile ();
    /// Update all settings, e.g. for logging level, by calling prefsFuncInt
    void UpdateCfgVals ();
    /// Read version numbers into verXplane/verXPLM
    void ReadVersions ();
    /// Set current thread as main xp Thread
    void ThisThreadIsXP()
    { xpThread = std::this_thread::get_id(); pluginId = XPLMGetMyID(); }
    /// Is this thread XP's main thread?
    bool IsXPThread() const { return std::this_thread::get_id() == xpThread; }

};

/// The one and only global variable structure
extern GlobVars glob;

