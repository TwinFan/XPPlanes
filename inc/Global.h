/// @file       Global.h
/// @brief      Defines the GlobVars class holding all global variable information
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

/// All global config settings and variables are kept in one structure for convenient access and central definition
struct GlobVars {
public:
    // MARK: Config File Settings
    // Config data shall be of type int, bool, or std::string
    
    /// Logging level
    logLevelTy      logLvl =
#if XPPLANES_VER_MAJOR == 0
                              logDEBUG;
#else
                              logINFO;
#endif
    /// Debug model matching?
    bool            bLogMdlMatch= false;
    /// Clamp all planes to the ground? Default is `false` as clamping is kinda expensive due to Y-Testing; then clamping is only activated when plane is thought to be on the ground
    bool            bClampAll   = false;
    /// Replace dataRefs in `.obj` files on load?
    bool            bObjReplDataRefs = true;
    /// Replace textures in `.obj` files on load if needed?
    bool            bObjReplTextures = true;
    
    /// Max distance from camera [nm]
    int             maxPlaneDist = 50;
// TODO: Allow for a buffering period
    /// Remove a plane after how many seconds without fresh data?
    int             outdatedPeriod = 30;
    /// Shall we draw aircraft labels?
    bool            bDrawLabels = true;
    /// Maximum distance for drawing labels? [m], defaults to 3nm
    int             maxLabelDist = 5556;
    /// Cut off labels at XP's reported visibility mit?
    bool            bLabelCutOffAtVisibility = true;
    
    /// Do we want to control X-Plane's AI/Multiplayer planes for TCAS?
    bool            bAITcasControl  = true;
    
    /// Do we feed X-Plane's maps with our aircraft positions?
    bool            bMapEnabled     = true;
    /// Do we show labels with the aircraft icons?
    bool            bMapLabels      = true;
    
    /// @brief The multicast group that we use, which is the same X-Plane is using itself for its BEACON
    /// @see <X-Plane>/Instructions/Exchanging Data with X-Plane.rtfd, chapter "DISCOVER X-PLANE BY A BEACON"
    std::string     listenMCGroup   = "239.255.1.1";
    /// The port we use is _different_ from the port the X-Plane BEACON uses, so we don't get into conflict
    int             listenMCPort    = 49900;
    /// The port for receiving UDP broadcast messages
    int             listenBcstPort  = 49800;
    /// Time-to-live, or mumber of hops for a multicast message
    int             remoteTTL       = 8;
    /// Buffer size, ie. max message length we send over multicast
    int             remoteBufSize   = 8192;

    // MARK: Dynamic Data
    
    /// Global map of all created planes
    mapPlanesTy     mapPlanes;
    /// Global map of available (potentially future) flight data
    mapListFlightDataTy mapListFD;
    /// Mutex protecting access to the above mapListFD
    std::mutex      mtxListFD;
    
    /// This plugin's id
    XPLMPluginID    pluginId        = 0;
    /// id of X-Plane's thread (when it is OK to use XP API calls)
    std::thread::id xpThread;
    /// Current X-Xplane's time
    float           now             = NAN;
    /// plugin's menu handle
    XPLMMenuID      hMenu           = NULL;
    
    /// Plugin status (type definition)
    enum StatusTy {
        STATUS_INACTIVE = 0,        ///< plugin inactive or starting up
        STATUS_WAITING,             ///< waiting for first network messages
        STATUS_ACTIVE,              ///< receiving data, displaying planes
    };
    /// Plugin status
    volatile StatusTy eStatus = STATUS_INACTIVE;

public:
    /// Constructor
    GlobVars (logLevelTy _logLvl = logINFO, bool _logMdlMatch = false) :
    logLvl(_logLvl), bLogMdlMatch(_logMdlMatch) {}
    
    /// Read from a config file
    bool ConfigFileLoad ();
    /// Write to a config file
    bool ConfigFileSave ();
    /// Set current thread as main xp Thread
    
    void ThisThreadIsXP()
    { xpThread = std::this_thread::get_id(); pluginId = XPLMGetMyID(); }
    /// Is this thread XP's main thread?
    bool IsXPThread() const { return std::this_thread::get_id() == xpThread; }

};

/// The one and only global variable structure
extern GlobVars glob;
