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
#include <memory>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <fstream>
#include <stdexcept>

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
#include "Network.h"
#include "Listener.h"
#include "FlightData.h"
#include "Plane.h"
#include "Global.h"
