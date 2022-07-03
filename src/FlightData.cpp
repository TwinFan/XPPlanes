/// @file       FlightData.cpp
/// @brief      Position, attitude, and configuration data that drives the planes' display
/// @details    This data is passed from the network thread to the main thread.
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

//
// MARK: Object Creation
//

// Constructor: Creates a FlightData object from received network data
FlightData::FlightData (const std::string& s)
{
    if (!FillFromNetworkData(s)) {
        throw(FlightData_error("Couldn't interpret network data as FlightData"));
    }
}

// Has usable data? (Has at least position information)
bool FlightData::IsUsable () const
{
    return
    _modeS_id != 0 &&                       // has an id
    !std::isnan(lat) &&                     // has a position
    !std::isnan(lon) &&
    (!std::isnan(alt_m) || bGnd);           // and altitude information
}

// Convert to XP's drawInfo
FlightData::operator XPLMDrawInfo_t () const
{
    double X, Y, Z;
    XPLMWorldToLocal(lat, lon, NZ(alt_m),
                     &X, &Y, &Z);
    return XPLMDrawInfo_t { sizeof(XPLMDrawInfo_t),
                            float(X), float(Y), float(Z),
                            NZ(pitch), NZ(heading), NZ(roll) };
}

// Reads flight data from the passed-in network data, identifying the type of data, then calling the appropriate conversion function
bool FlightData::FillFromNetworkData (const std::string& s)
{
    // Try the different formats the we support
    if (FillFromRTTFC(s))
    {
        // One of the format accepted the input. Was it sufficiently detailed?
        if (!IsUsable()) {
            LOG_MSG(logDEBUG, "Not enough informaton in the data to be usable");
            return false;
        }
        return true;
    }
    LOG_MSG(logDEBUG, "No format converter could make use of the data");
    return false;
}


//
// MARK: Global Functions
//

// Initialize the FlightData module
bool FlightDataStartup ()
{
    // doesn't do anything at the moment
    return true;
}

// Shutdown the FlightData module
void FlightDataShutdown ()
{
    // cleanup all data in the map
    glob.mapListFD.clear();
}
