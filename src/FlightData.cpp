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

// Main function to interpret network data
bool FlightData::ProcessNetworkData (const std::string& s)
{
    // Determin type of data
    std::size_t p = s.find_first_of("[{,");
    if (p == std::string::npos) {           // eh, what? no JSON, no CSV
        LOG_MSG(logDEBUG, "Not identified as either JSON or CSV");
        return false;
    }
    const bool bJson = (s[p] != ',');       // is probably JSON?
    
    // If JSON then we try to parse it right here already
    JsonRoot jsonRoot (bJson ? s.c_str() : nullptr);
    if (bJson && !jsonRoot) {
        LOG_MSG(logWARN, "Looks like JSON but couldn't be parsed:\n%.80s", s.c_str());
        return false;
    }
    
    // Act according to type of data
    try {
        switch (s[p]) {
            // Array-style JSON data, so this is multiple records that we need to loop over
            case '[':
            {
                // Find the array
                JSON_Array* pArr = json_array(jsonRoot);
                if (!pArr) {
                    LOG_MSG(logWARN, "Couldn't find array object in parsed JSON data:\n%.80s", s.c_str());
                    return false;
                }
                // Loop over all array values and interpret them as flight data
                bool bRet = true;
                for (size_t i = 0; i < json_array_get_count(pArr); ++i)
                {
                    JSON_Object* pObj = json_array_get_object(pArr, i);
                    if (!pObj) {
                        LOG_MSG(logWARN, "Couldn't find root object in parsed JSON array, index %lu:\n%.80s", (unsigned long)i, s.c_str());
                        bRet = false;
                    } else {
                        if (!AddNew(std::make_shared<FlightData>(pObj)))
                            bRet = false;
                    }
                }
                return bRet;
            }
                
            // Single-record-style JSON data
            case '{':
            {
                JSON_Object* pObj = json_object(jsonRoot);
                if (!pObj) {
                    LOG_MSG(logWARN, "Couldn't find root object in parsed JSON data:\n%.80s", s.c_str());
                    return false;
                }
                return AddNew(std::make_shared<FlightData>(pObj));
            }
                
            // Single-record-style CSV data, calls constructor with std::string parameter
            case ',':
                return AddNew(std::make_shared<FlightData>(s));
        }
    }
    catch (const FlightData_error& e) {
        LOG_MSG(logDEBUG, "Couldn't convert to FlightData object, unknown format or data insufficient:\n%.80s",
                s.c_str());
        return false;
    }
    catch (const std::exception& e) {
        LOG_MSG(logWARN, "Couldn't convert to FlightData object, %s:\n%.80s",
                e.what(), s.c_str());
        return false;
    }
    return true;
}

// Add a just created object to the internal list
bool FlightData::AddNew (std::shared_ptr<FlightData>&& pFD)
{
    // *** Timestamp ***
    
    // If no timestamp was given we assume 'now'
    if (!pFD->ts.time_since_epoch().count())
        pFD->ts = std::chrono::system_clock::now();
    
    // Add the buffering period to the timestamp
    pFD->ts += std::chrono::seconds(glob.bufferPeriod);
    
    // One of the format accepted the input. Was it sufficiently detailed?
    if (!pFD->IsUsable()) {
        pFD = nullptr;                          // destroy FlightData object
        throw(FlightData_error("Not enough informaton in the data to be usable"));
        return false;
    }
    
    // *** Add Data ***

    // Discard data if already older than grace period
    if (pFD->ts <= std::chrono::system_clock::now() - std::chrono::seconds(glob.gracePeriod)) {
        LOG_MSG(logDEBUG, "Ignoring too old data for %06X from %.1fs ago", pFD->_modeS_id,
                std::chrono::duration<double>(std::chrono::system_clock::now() - pFD->ts).count());
        pFD = nullptr;
        // But even then we return `true` as the data as such was OK...just was too late
        return true;
    }
    
    // insertion into the map/list of flight data is protected by a mutex
    std::lock_guard<std::mutex> guard(glob.mtxListFD);
    listFlightDataTy& listFD = glob.mapListFD[pFD->_modeS_id];
    if (listFD.empty() ||
        listFD.back()->ts + MIN_TS_DIFF <= pFD->ts)  // only add if data is newer, this way it stays sorted
        listFD.emplace_back(std::move(pFD));
    else {
        LOG_MSG(logDEBUG,
                listFD.back()->ts == pFD->ts ?
                "Ignoring same-timstamp data for %06X, ts = %ld" :
                "Ignoring out of sequence data for %06X, ts = %ld", pFD->_modeS_id,
                (long)pFD->ts.time_since_epoch().count());
        pFD = nullptr;
    }
    
    return true;
}

// Constructor: Creates a FlightData object from single record CSV-style data
FlightData::FlightData (const std::string& csv)
{
    if (!FillFromRTTFC(csv)) {
        throw(FlightData_error("Couldn't interpret network data as RTTFC"));
    }
}

// Constructor: Creates a FlightData object from a JSON object
FlightData::FlightData (const JSON_Object* obj)
{
    if (!FillFromXPPTraffic(obj)) {
        throw(FlightData_error("Couldn't interpret network data as XPPTraffic"));
    }
}

// Set timestamp from input value
void FlightData::SetTimestamp (double tsIn)
{
    // Consider NAN = 0.0 = "now"
    if (std::isnan(tsIn))
        tsIn = 0.0;
    // Considered a Java timestamp in milliseconds?
    if (tsIn >= 1577836800000.0)
        tsIn /= 1000.0;                 // turn it into a Unix timestamp with decimals
    // Absolute Unix timestamp?
    if (tsIn >= 1577836800.0) {
        // convert to system clock, truncating the decimals
        ts = std::chrono::system_clock::from_time_t(time_t(tsIn));
        // now add the fractional part back in
        tsIn -= floor(tsIn);
        tsIn *= 1000.0;
        ts += std::chrono::milliseconds(long(tsIn));
    }
    // Relative Timestamp
    else {
        ts = std::chrono::system_clock::now();
        ts += std::chrono::milliseconds(long(tsIn*1000.0));
    }
}

//
// MARK: Utility Functions
//

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

// Replace any remaining `NAN`s with `0.0`
void FlightData::NANtoZero ()
{
#define NAN2Z(v) if (std::isnan(v)) v = 0.0f;
    NAN2Z(pitch);
    NAN2Z(heading);
    NAN2Z(roll);
    NAN2Z(gear);
    NAN2Z(nws);
    NAN2Z(flaps);
    NAN2Z(spoilers);
    NAN2Z(reversers);
    NAN2Z(thrust);
    NAN2Z(engineRpm);
    // we specifically do not touch `wake`, ie. NAN can remain there as they are handled by XPMP2
}

/// Replace any remaining `NAN`s with values from the other object
void FlightData::NANtoCopy (const FlightData& o)
{
#define NAN2CPY(v) if (std::isnan(v)) v = o.v;
    NAN2CPY(pitch);
    NAN2CPY(heading);
    NAN2CPY(roll);
    NAN2CPY(gear);
    NAN2CPY(nws);
    NAN2CPY(flaps);
    NAN2CPY(spoilers);
    NAN2CPY(reversers);
    NAN2CPY(thrust);
    NAN2CPY(engineRpm);
    // but unlike in FlightData::NANtoZero() we do copy wake information from previous;
    // this might copy NAN...but that's OK, but once given from outside we keep copying those values
    NAN2CPY(wake.wingSpan_m);
    NAN2CPY(wake.wingArea_m2);
    NAN2CPY(wake.mass_kg);
    // we do specifically _not_ copy wake.lift, ie. if list is no longer given then we return to defaults
    
    // We do copy the label as it receives special treatment when processing the data
    if (label.empty()) label = o.label;
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
