/// @file       FlightData.h
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

#pragma once

//
// MARK: Flight Data
//

/// Timestamp format is a system clock's timepoint
typedef std::chrono::time_point<std::chrono::system_clock> tsTy;

/// Transports flight data for location, attitude, configuration between the network and the main thread
class FlightData
{
public:
    // Key and identification
    XPMPPlaneID _modeS_id = 0;      ///< key
    std::string icaoType;           ///< ICAO aircraft type according to doc8643
    std::string icaoAirline;        ///< ICAO airline code (for model matching)
    std::string livery;             ///< special livery code (optional, for model matching)
    
    // Validity
    tsTy        ts;                 ///< timestamp
    
    // Location
    double      lat         = NAN;  ///< latitude
    double      lon         = NAN;  ///< longitude
    double      alt_m       = NAN;  ///< altitude in meter above ground
    bool        bGnd        = false;///< on the ground?
    
    // Attitude
    float       pitch       = NAN;  ///< Pitch in degres to rotate the object, positive is up.
    float       heading     = NAN;  ///< Heading in local coordinates to rotate the object, clockwise
    float       roll        = NAN;  ///< Roll to rotate the object
    
    /// Wake turbulence calculation data: wing span, wing area, aircraft mass
    XPMP2::Aircraft::wakeTy wake;
    
    // Configuration
    float       gear        = NAN;  ///< gear down = 1.0, gear up = 0.0
    float       nws         = NAN;  ///< nose wheel steering degree, 0.0 = straight ahead, negative = left
    float       flaps       = NAN;  ///< flaps deployed = 1.0, flaps up = 0.0
    float       spoilers    = NAN;  ///< spoilers (speedbrakes) up = 1.0, down = 0.0

    /// Aircraft lights
    struct lightsTy {
        bool taxi       : 1;        ///< taxi lights
        bool landing    : 1;        ///< landing lights
        bool beacon     : 1;        ///< beacon lights
        bool strobe     : 1;        ///< strobe lights
        bool nav        : 1;        ///< navigation lights
    } lights = { false, false, false, false, false };
    
    // TODO: Reasonable approach to engine, rotor, wheels

public:
    /// Constructor: Creates a FlightData object from received network data
    FlightData (const std::string& s);
    
    /// Order is solely by timestamp
    bool operator< (const FlightData& o) const { return ts < o.ts; }
    
    /// Has usable data? (Has at least position information)
    bool IsUsable () const;
    
    /// Convert to XP's drawInfo
    operator XPLMDrawInfo_t () const;
    
protected:
    /// @brief Reads flight data from the passed-in network data, identifying the type of data, then calling the appropriate conversion function
    /// @returns if any converter worked and produced usable data
    bool FillFromNetworkData (const std::string& s);
    
    // The following format-handling functions shall
    // 1. verify if the format matches expectation, if not exit quickly,
    // 2. fill fields from the data
    // Return if conversion was technically successful
    
    /// @brief RTTFC: Interprets the data as an RTTFC line
    /// @see https://www.flyrealtraffic.com/RTdev2.0.pdf
    bool FillFromRTTFC (const std::string& s);
};

/// Smart pointer to flight data objects
typedef std::shared_ptr<FlightData> ptrFlightDataTy;

/// List of flight data elements
typedef std::list<ptrFlightDataTy> listFlightDataTy;

/// Map indexed by plane id holding lists of flight data elements
typedef std::map<XPMPPlaneID,listFlightDataTy> mapListFlightDataTy;

//
// MARK: Exception class
//

/// Exception thrown by FlightData, e.g. when object creation impossible
class FlightData_error : public std::runtime_error {
public:
    /// Constructor takes some error message
    FlightData_error (const std::string& what_arg) :
    std::runtime_error(what_arg) {}
};

//
// MARK: Global Functions
//

/// Initialize the FlightData module
bool FlightDataStartup ();

/// Shutdown the FlightData module
void FlightDataShutdown ();
