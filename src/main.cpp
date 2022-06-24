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

#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMMenus.h"
#include "XPLMPlanes.h"
#include "XPLMDataAccess.h"
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#if IBM
	#include <windows.h>
#endif

// OpenGL is system-dependent
#include "OpenGL/SystemGL.h"

#ifndef XPLM300
	#error This is made to be compiled against the XPLM300 SDK
#endif

// XPPlanes' definitions
#include "Constants.h"


PLUGIN_API int XPluginStart(
							char *		outName,
							char *		outSig,
							char *		outDesc)
{
    snprintf(outName, 100, XPPLANES "v%d.%d.%d", XPPLANES_VER_MAJOR, XPPLANES_VER_MINOR, XPPLANES_VER_PATCH);
	strcpy(outSig, "twinfan.plugin." XPPLANES);
	strcpy(outDesc, "Display additional planes controled by network messages");
	
	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
}

PLUGIN_API void XPluginDisable(void)
{
}


PLUGIN_API int  XPluginEnable(void)
{
    return 1;
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * /*inParam*/)
{
    if (inMsg != XPLM_MSG_RELEASE_PLANES)
        return;
    
    char who[256] = "?";
    XPLMGetPluginInfo(inFrom, who, NULL, NULL, NULL);
    char msg[1000];
    snprintf(msg,sizeof(msg),"Hello TCAS: '%s' (id %d) requested us to release TCAS!\n",
             who, inFrom);
    XPLMDebugString(msg);
    
    snprintf(msg,sizeof(msg),"'%s' (id %d) requested us to release TCAS!\n",
             who, inFrom);
}
