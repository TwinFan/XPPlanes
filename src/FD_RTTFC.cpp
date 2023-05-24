/// @file       FD_RTTFC.cpp
/// @brief      Converter for RTTFC type of data
/// @details    RTTFC is a CSV-style format introduced by RealTraffic in v9
/// @see        https://www.flyrealtraffic.com/RTdev2.0.pdf
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

/// @brief Fields in a RealTraffic RTTFC message (since v9 on port 49005)
/// @see `LiveTraffic/Inc/LTRealTraffic.h`
enum RT_RTTFC_FIELDS_TY : int {
    RT_RTTFC_REC_TYPE = 0,          ///< "RTTFC"
    RT_RTTFC_HEXID,                 ///< transponder hex code, converted to decimal
    RT_RTTFC_LAT,                   ///< latitude in degrees
    RT_RTTFC_LON,                   ///< longitude in degrees
    RT_RTTFC_ALT_BARO,              ///< altitude in feet (barometric, not adapted for local pressure)
    RT_RTTFC_BARO_RATE,             ///< barometric vertical rate
    RT_RTTFC_GND,                   ///< ground flag
    RT_RTTFC_TRACK,                 ///< track
    RT_RTTFC_GSP,                   ///< ground speed
    RT_RTTFC_CS_ICAO,               ///< ICAO call sign
    RT_RTTFC_AC_TYPE,               ///< aircraft type
    RT_RTTFC_AC_TAILNO,             ///< aircraft registration
    RT_RTTFC_FROM_IATA,             ///< origin IATA code
    RT_RTTFC_TO_IATA,               ///< destination IATA code
    RT_RTTFC_TIMESTAMP,             ///< unix epoch timestamp when data was last updated
    RT_RTTFC_SOURCE,                ///< data source
    RT_RTTFC_CS_IATA,               ///< IATA call sign
    RT_RTTFC_MSG_TYPE,              ///< type of message
    RT_RTTFC_ALT_GEOM,              ///< geometric altitude (WGS84 GPS altitude)
    RT_RTTFC_IAS,                   ///< indicated air speed
    RT_RTTFC_TAS,                   ///< true air speed
    RT_RTTFC_MACH,                  ///< Mach number
    RT_RTTFC_TRACK_RATE,            ///< rate of change for track
    RT_RTTFC_ROLL,                  ///< roll in degrees, negative = left
    RT_RTTFC_MAG_HEADING,           ///< magnetic heading
    RT_RTTFC_TRUE_HEADING,          ///< true heading
    RT_RTTFC_GEOM_RATE,             ///< geometric vertical rate
    RT_RTTFC_EMERGENCY,             ///< emergency status
    RT_RTTFC_CATEGORY,              ///< category of the aircraft
    RT_RTTFC_NAV_QNH,               ///< QNH setting navigation is based on
    RT_RTTFC_NAV_ALTITUDE_MCP,      ///< altitude dialled into the MCP in the flight deck
    RT_RTTFC_NAV_ALTITUDE_FMS,      ///< altitude set by the flight management system (FMS)
    RT_RTTFC_NAV_HEADING,           ///< heading set by the MCP
    RT_RTTFC_NAV_MODES,             ///< which modes the autopilot is currently in
    RT_RTTFC_SEEN,                  ///< seconds since any message updated this aircraft state vector
    RT_RTTFC_RSSI,                  ///< signal strength of the receiver
    RT_RTTFC_WINDDIR,               ///< wind direction in degrees true north
    RT_RTTFC_WINDSPD,               ///< wind speed in kts
    RT_RTTFC_OAT,                   ///< outside air temperature / static air temperature
    RT_RTTFC_TAT,                   ///< total air temperature
    RT_RTTFC_ISICAOHEX,             ///< is this hexid an ICAO assigned ID.
    RT_RTTFC_AUGMENTATION_STATUS,   ///< has this record been augmented from multiple sources
    RT_RTTFC_MIN_TFC_FIELDS         ///< always last, minimum number of fields
};

static const char* CSV_DELIM = ",";

#define TO_DOUBLE(v) v = std::stod(tok); break;
#define TO_FLOAT(v) v = std::stof(tok); break;
#define TO_STR(v) v = tok; break;

// RTTFC: Interprets the data as an RTTFC line
bool FlightData::FillFromRTTFC (const std::string& csv)
{
    // *** 1. Could it be our format? ***
    if (csv.substr(0,5) != "RTTFC")             // needs to start with 'RTTFC'
        return false;

    // *** 2. Convert ***
    double alt_baro_ft = NAN;
    double qnh = NAN;
    
    // Loop over all tokens, ie. elements in the list of CSV fields
    StrTokens t(csv, CSV_DELIM);
    while (!t.finished())
    {
        const std::string tok = t.next();
        const RT_RTTFC_FIELDS_TY num = RT_RTTFC_FIELDS_TY(t.count()-1);
        
        // There are a couple of indicators for a value that shall be ignored
        if (tok.empty()             ||          // empty string
            tok == "-1"             ||          // various representations of -1
            tok == "-1.0"           ||
            tok == "-1.00")
            continue;                           // -> skip
        
        try {
            switch (num) {
                case RT_RTTFC_REC_TYPE:             // must be RTTFC
                    if (tok != "RTTFC") {
                        LOG_MSG(logDEBUG, "Wrong record type: %s", tok.c_str());
                        return false;
                    }
                    break;
                case RT_RTTFC_HEXID:
                    _modeS_id = (XPMPPlaneID)std::stoul(tok, nullptr, 0);
                    break;
                case RT_RTTFC_LAT:          TO_DOUBLE(lat);
                case RT_RTTFC_LON:          TO_DOUBLE(lon);
                case RT_RTTFC_ALT_BARO:     TO_DOUBLE(alt_baro_ft);
                case RT_RTTFC_GND:
                    bGnd = std::stoi(tok) != 0;
                    if (bGnd) gear = 1.0f;              // on the ground need gear
                    break;
                case RT_RTTFC_CS_ICAO:                  // in lieu of airline take first 3 chars as airline
                    callSign = icaoAirline = tok;       // but also store the full call sign
                    if (icaoAirline.length() > 3)
                        icaoAirline.erase(3);
                    break;
                case RT_RTTFC_CS_IATA:                  // we prefer the ICAO version, so don't overwrite
                    if (callSign.empty()) callSign = tok;
                    break;
                case RT_RTTFC_AC_TYPE:      TO_STR(icaoType);
                case RT_RTTFC_AC_TAILNO:    TO_STR(tailNum);
                case RT_RTTFC_TIMESTAMP:
                    SetTimestamp(std::stod(tok));
                    break;
                case RT_RTTFC_ALT_GEOM:                 // altitude is given in feet, convert to meter
                    alt_m = std::stod(tok) * XPMP2::M_per_FT;
                    break;
                case RT_RTTFC_ROLL:         TO_FLOAT(roll);
                case RT_RTTFC_MAG_HEADING:  TO_FLOAT(heading);
                case RT_RTTFC_TRUE_HEADING: TO_FLOAT(heading);      // overwrites a mag heading, which is good
                case RT_RTTFC_NAV_QNH:      TO_DOUBLE(qnh);
                    
                default:                            // don't handle a couple of fields
                    break;
            }
        }
        // ignore number conversion exception...we'll just not set that field and see what happens
        catch (const std::invalid_argument& e) {}
        catch (const std::out_of_range& e) {}
    }
    
    // Altitude: if we didn't get an actual geo altitude we need to try deal with baro altitude
    if (std::isnan(alt_m) && !std::isnan(alt_baro_ft)) {
        if (!std::isnan(qnh))                   // if we have a QNH value we can even "convert" to geo altitude
            alt_baro_ft = WeatherAltCorr_ft(alt_baro_ft, qnh);
        alt_m = alt_baro_ft * XPMP2::M_per_FT;
    }
    
    // Lights: We just assume most on, landing lights below 10,000ft
    lights.defined  = true;
    lights.taxi     = bGnd;
    lights.beacon   = true;
    lights.landing  = alt_m < 10000.0 * XPMP2::M_per_FT;
    lights.nav      = true;
    lights.strobe   = true;
    
    return true;
}
