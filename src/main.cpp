/// @file       main.cpp
/// @brief      Plugin main entry points
/// @details    Has the mandatory X-Plane interface functions `XPlugin...`
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

PLUGIN_API int  XPluginEnable(void);
PLUGIN_API void XPluginDisable(void);

//
// MARK: XPMP2 Preferences
//

/// This is a callback the XPMP2 calls regularly to learn about configuration settings.
/// Only 3 are left, all of them integers.
int CBIntPrefsFunc (const char *, [[maybe_unused]] const char * item, int defaultVal)
{
    // We always want to replace dataRefs and textures upon load to make the most out of the .obj files
    if (!strcmp(item, XPMP_CFG_ITM_REPLDATAREFS))   return glob.bObjReplDataRefs;           // replace dataRefs in object files?
    if (!strcmp(item, XPMP_CFG_ITM_REPLTEXTURE))    return glob.bObjReplTextures;           // replace textures in object files=
    if (!strcmp(item, XPMP_CFG_ITM_CLAMPALL))       return glob.bClampAll;                  // never hide planes below the ground?
    if (!strcmp(item, XPMP_CFG_ITM_HANDLE_DUP_ID))  return 0;                               // don't expect duplicate ids
    if (!strcmp(item, XPMP_CFG_ITM_SUPPORT_REMOTE)) return -1;                              // We don't want this plugin to ever _send_ traffic!
    if (!strcmp(item, XPMP_CFG_ITM_LOGLEVEL))       return (int)glob.logLvl;                // logging level
    if (!strcmp(item, XPMP_CFG_ITM_MODELMATCHING))  return glob.bLogMdlMatch;               // log model matching
    // Otherwise we just accept defaults
    return defaultVal;
}

//
// MARK: TCAS Control
//

/// have we requested AI access and are now waiting for a callback?
static bool bWaitingForAI = false;

void ClientTryGetAI ();

/// Callback for requesting TCAS: Maybe now we have a chance?
void ClientCBRetryGetAI (void*)
{
    bWaitingForAI = false;
    
    // If we still want to have TCAS we just try again
    if (glob.bAITcasControl && !XPMPHasControlOfAIAircraft())
        ClientTryGetAI();
}

// Try getting TCAS/AI control
void ClientTryGetAI ()
{
    // make sure we do this from the main thread only!
    if (!glob.IsXPThread()) return;
    
    // no need to try (again)?
    if (bWaitingForAI || XPMPHasControlOfAIAircraft())
        return;
    
    // Try getting AI control, pass callback for the case we couldn't get it
    const char* cszResult = XPMPMultiplayerEnable(ClientCBRetryGetAI);
    if ( cszResult[0] ) {
        bWaitingForAI = true;
        LOG_MSG(logWARN, "%s", cszResult);
    } else if (XPMPHasControlOfAIAircraft()) {
        bWaitingForAI = false;
        LOG_MSG(logINFO, "Have TCAS / AI control now");
    }
}

// Stop TCAS/AI control
void ClientReleaseAI ()
{
    XPMPMultiplayerDisable();
    bWaitingForAI = false;
}

//
// MARK: Menu
//

// menu indexes
constexpr std::uintptr_t MENU_ACTIVE = 0;
constexpr std::uintptr_t MENU_TCAS   = 1;

/// Command definition per menu item
struct CmdMenuDefTy {
    const char* cmdName = nullptr;          ///< command's name
    const char* menuName = nullptr;         ///< (initial) menu item's name
    const char* description = nullptr;      ///< human-readable command description
    XPLMCommandRef hCmd = nullptr;          ///< command reference assigned by X-Plane
} CMD_MENU_DEF[2] = {
    { XPPLANES "/Activate",  "Active",       "Toggle if " XPPLANES " shall display planes" },
    { XPPLANES "/TCAS",      "TCAS Control", "Toggle if " XPPLANES " shall have TCAS control" },
};

/// Sets all menu checkmarks according to current status
void MenuUpdateCheckmarks ()
{
    // Menu item "Active"
    switch (glob.eStatus) {
        case GlobVars::STATUS_WAITING:
            XPLMSetMenuItemName(glob.hMenu, MENU_ACTIVE, "Active (waiting for data)", 0);
            XPLMCheckMenuItem(glob.hMenu, MENU_ACTIVE, xplm_Menu_Checked);
            break;

        case GlobVars::STATUS_ACTIVE: {
            char s[50];
            snprintf (s, sizeof(s), "Active (%ld aircraft)", XPMPCountPlanes());
            XPLMSetMenuItemName(glob.hMenu, MENU_ACTIVE, s, 0);
            XPLMCheckMenuItem(glob.hMenu, MENU_ACTIVE, xplm_Menu_Checked);
            break;
        }
            
        default:
            XPLMSetMenuItemName(glob.hMenu, MENU_ACTIVE, "Activate (currently inactive)", 0);
            XPLMCheckMenuItem(glob.hMenu, MENU_ACTIVE, xplm_Menu_Unchecked);
            break;
    }
    
    // Menu item "TCAS Control" (status display)
    XPLMCheckMenuItem(glob.hMenu, MENU_TCAS, XPMPHasControlOfAIAircraft() ? xplm_Menu_Checked : xplm_Menu_Unchecked);
}

/// Callback function for menu
int CmdCallback (XPLMCommandRef cmdRef, XPLMCommandPhase inPhase, void*)
{
    // entry point into plugin...catch exceptions latest here
    try {
        if (inPhase == xplm_CommandBegin) {
            if (cmdRef == CMD_MENU_DEF[MENU_ACTIVE].hCmd) {
                // Toggle activation of plugin
                if (glob.eStatus == GlobVars::STATUS_INACTIVE)
                    XPluginEnable();
                else
                    XPluginDisable();
            }
            else if (cmdRef == CMD_MENU_DEF[MENU_TCAS].hCmd) {
                // Toggle TCAS/AI status
                if (XPMPHasControlOfAIAircraft())
                    ClientReleaseAI();
                else
                    ClientTryGetAI();
            }
            
            // Update check marks...things might have changed
            MenuUpdateCheckmarks();
        }
    }
    catch (const std::exception& e) {
        LOG_MSG(logFATAL, ERR_EXCEPTION, e.what());
    }
    return 1;
}

//
// MARK: Flight Loop
//

/// ID of our flight loop callback for regular tasks
XPLMFlightLoopID flId = nullptr;

/// Regular tasks, called by flight loop
float FlightLoop_EverySecond(float, float, int, void*)
{
    // entry point into plugin...catch exceptions latest here
    try {
        GetMiscNetwTime();              // update rcGlob.now, e.g. for logging from worker threads
        PlaneMaintenance();             // regular plane updates from flight data
        MenuUpdateCheckmarks();         // update menu
    }
    catch (const std::exception& e) {
        LOG_MSG(logFATAL, ERR_EXCEPTION, e.what());
    }
    
    // call me every other flight loop
    return -2.0f;
}

//
// MARK: Plugin Callbacks
//


/// @brief X-Plane plugin standard startup function
/// @see https://developer.x-plane.com/article/developing-plugins/#XPluginStart
PLUGIN_API int XPluginStart(char *		outName,
							char *		outSig,
							char *		outDesc)
{
    // this is the XP main thread
    glob.ThisThreadIsXP();
    GetMiscNetwTime();
    glob.ConfigFileLoad();              // Read configuration file
#ifdef DEBUG
    glob.logLvl = logDEBUG;
#endif
    
    snprintf(outName, 100, XPPLANES " v%d.%d.%d", XPPLANES_VER_MAJOR, XPPLANES_VER_MINOR, XPPLANES_VER_PATCH);
    strcpy(outSig, "twinfan.plugin." XPPLANES);
    strcpy(outDesc, "Display additional planes controled by network messages");

    LOG_MSG(logMSG, "%s starting up...", outName);

    // use native paths, i.e. Posix style (as opposed to HFS style)
    // https://developer.x-plane.com/2014/12/mac-plugin-developers-you-should-be-using-native-paths/
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS",1);

    // *** XPMP2 and CSL models ***
    
    // The path separation character, one out of /\:
    char pathSep = XPLMGetDirectorySeparator()[0];
    // The plugin's path, results in something like ".../Resources/plugins/XPPlanes/mac_x64/XPPlanes.xpl"
    char szPath[256];
    XPLMGetPluginInfo(glob.pluginId, nullptr, szPath, nullptr, nullptr);
    *(std::strrchr(szPath, pathSep)) = 0;   // Cut off the plugin's file name
    *(std::strrchr(szPath, pathSep) + 1) = 0; // Cut off the platform directory ("mac_x64") name, but leave the dir separation character
    // We search in a subdirectory named "Resources" for all we need
    std::string resourcePath = szPath;
    resourcePath += "Resources";            // should now be something like ".../Resources/plugins/XPPlanes/Resources"

    // Try initializing XPMP2:
    const char* res = XPMPMultiplayerInit(XPPLANES,             // plugin name,
                                          resourcePath.c_str(), // path to supplemental files
                                          CBIntPrefsFunc,       // configuration callback function
                                          "A320",               // default ICAO type
                                          XPPLANES);            // plugin short name
    
    // Any error?
    if (res[0]) {
        LOG_MSG(logFATAL, "Initialization of XPMP2 failed: %s", res);
    }
    else {
        // Load our CSL models
        res = XPMPLoadCSLPackage(resourcePath.c_str());     // CSL folder root path
        if (res[0]) {
            LOG_MSG(logERR, "Error while loading CSL packages: %s", res);
        }
        
        // Set some generic configurations
        XPMPSetAircraftLabelDist(glob.maxLabelDist, glob.bLabelCutOffAtVisibility);
        XPMPEnableAircraftLabels(glob.bDrawLabels);
    }
    
    // *** Plugin's menu ***

    // Create the menu for the plugin
    int my_slot = XPLMAppendMenuItem(XPLMFindPluginsMenu(), XPPLANES, NULL, 0);
    glob.hMenu = XPLMCreateMenu(XPPLANES, XPLMFindPluginsMenu(), my_slot, NULL, NULL);

    // No CSL models installed?
    if (XPMPGetNumberOfInstalledModels() <= 0) {
        XPLMAppendMenuItem(glob.hMenu, "Disabled - No CSL models installed!", (void*)MENU_ACTIVE, 0);
        XPLMEnableMenuItem(glob.hMenu, 0, false);
        LOG_MSG(logFATAL, "There are no CSL models installed, " XPPLANES " CANNOT START!");
        LOG_MSG(logFATAL, "Make sure to install a few CSL models under %s", resourcePath.c_str());
        // We still return 1...so the menu item is available and visible to the user - otherwise the plugin would disappear with no trace except log entries
        return 1;
    }

    // Define "proper" command and menu items
    for (CmdMenuDefTy& cmdDef: CMD_MENU_DEF) {
        cmdDef.hCmd = XPLMCreateCommand(cmdDef.cmdName, cmdDef.description);
        XPLMRegisterCommandHandler(cmdDef.hCmd, CmdCallback, 1, NULL);
        XPLMAppendMenuItemWithCommand(glob.hMenu, cmdDef.menuName, cmdDef.hCmd);
    }

    MenuUpdateCheckmarks();

    return 1;
}

/// @brief Enable actual functionality of the plugin
/// @see https://developer.x-plane.com/article/developing-plugins/#XPluginEnable
PLUGIN_API int  XPluginEnable(void)
{
    // No CSL models installed? Then don't try starting
    if (XPMPGetNumberOfInstalledModels() <= 0) {
        return 1;
    }
    
    // Startup all modules, bail if one fails
    if (!PlaneStartup() ||
        !FlightDataStartup() ||
        !ListenStartup())
    {
        LOG_MSG(logFATAL, "One of the modules didn't startup, can't run!");
        XPluginDisable();
        return 0;
    }

    // Create a flight loop callback for some regular tasks, called every second
    XPLMCreateFlightLoop_t flParams = {
        sizeof(flParams),                           // structSize
        xplm_FlightLoop_Phase_BeforeFlightModel,    // phase
        FlightLoop_EverySecond,                     // callbackFunc,
        nullptr                                     // refcon
    };
    flId = XPLMCreateFlightLoop(&flParams);
    XPLMScheduleFlightLoop(flId, 1.0f, true);
    
    // Update the menus
    MenuUpdateCheckmarks();
    
    // Success
    LOG_MSG(logINFO, "Enabled");
    return 1;
}

/// @brief Disable functionality, cleanup runtime resources
/// @see https://developer.x-plane.com/article/developing-plugins/#XPluginDisable
PLUGIN_API void XPluginDisable(void)
{
    // Stop our flight loop callback
    if (flId)
        XPLMDestroyFlightLoop(flId);
    flId = nullptr;

    // Shutdown and cleanup all modules
    ListenShutdown();
    FlightDataShutdown();
    PlaneShutdown();
    
    // Write config file
    glob.ConfigFileSave();

    // Update the menus
    MenuUpdateCheckmarks();

    LOG_MSG(logINFO, "Disabled");
}

/// @brief Properly shutdown the plugin, cleanup last resources
/// @see https://developer.x-plane.com/article/developing-plugins/#XPluginStop
PLUGIN_API void XPluginStop(void)
{
    // Properly clean up
    XPMPMultiplayerCleanup();
}

/// @brief  Process the message that some other plugin wants TCAS control
/// @see https://developer.x-plane.com/article/developing-plugins/#XPluginReceiveMessage
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * /*inParam*/)
{
    // We only process one message: Did someone else wants to have TCAS control?
    if (inMsg != XPLM_MSG_RELEASE_PLANES)
        return;

    // Who is it that wants TCAS control?
    char who[256] = "?";
    XPLMGetPluginInfo(inFrom, who, NULL, NULL, NULL);
    LOG_MSG(logMSG, "'%s' (id %d) requested us to release TCAS, so we do",
            who, inFrom);
    
    // Release AI/TCAS
    ClientReleaseAI();
}
