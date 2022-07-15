/// @file       FD_XPPTraffic.cpp
/// @brief      XPPTraffic data format, a JSON format specifically designed for XPPlanes
/// @details    A flight data record in XPPTraffic format looks like this:
///             @code{.json}
///                {
///                  "id" : 4711,
///                  "ident" : {
///                    "airline" : "DLH",
///                    "reg" : "D-EVEL",
///                    "call" : "DLH1234"
///                  },
///                  "type" : {
///                    "icao" : "C172",
///                    "wingSpan" : 11.1,
///                    "wingArea" : 16.2
///                  },
///                  "position" : {
///                    "lat" : 51.406292,
///                    "lon" : 6.939847,
///                    "alt_geo" : 407,
///                    "gnd" : true
///                  },
///                  "attitude" : {
///                    "roll" : -0.2,
///                    "heading" : 42,
///                    "pitch" : 0.1
///                  },
///                  "config" : {
///                    "mass" : 1037.6,
///                    "lift" : 10178.86,
///                    "gear" : 1,
///                    "noseWheel" : -2.5,
///                    "flaps" : 0.5,
///                    "spoiler" : 0
///                  },
///                  "light" : {
///                    "taxi" : true,
///                    "landing" : false,
///                    "beacon" : true,
///                    "strobe" : false,
///                    "nav" : true
///                  }
///                }
///             @endcode
///             Alternatively, several records can be sent in a JSON array:
///             @code{.json}
///             [
///                 { "id" : 4711, ... },
///                 { "id" : 0815, ... },
///                 { "id" : 1234, ...}
///             ]
///             @endcode
///             Only changed attributes need to be sent, so the full info
///             is only needed in the first record.
/// @author     Birger Hoppe
/// @copyright  (c) 2020 Birger Hoppe
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

/// Converts the purpose-desgined XPPTraffic JSON format
bool FlightData::FillFromXPPTraffic (const JSON_Object* obj)
{
    // `id` is mandatory, otherwise I wouldn't know for which plane
    _modeS_id = XPMPPlaneID(jog_l(obj, "id"));
    JSON_Object* pSub = json_object_get_object(obj, "position");
    if (!_modeS_id || !pSub) {
        LOG_MSG(logWARN, "JSON record is missing the `id` attribute or the `position` object");
        return false;
    }
    
    // Position
    lat                 = jog_n_nan (pSub, "lat");
    lon                 = jog_n_nan (pSub, "lon");
    alt_m               = jog_n_nan (pSub, "alt_geo") * XPMP2::M_per_FT;
    bGnd                = jog_b     (pSub, "gnd");
    SetTimestamp(jog_n_nan(pSub, "timestamp"));
    
    // ident
    if ((pSub = json_object_get_object(obj, "ident"))) {
        icaoAirline     = jog_s     (pSub, "airline");
        livery          = jog_s     (pSub, "reg");
        callSign        = jog_s     (pSub, "call");
    }

    // type
    if ((pSub = json_object_get_object(obj, "type"))) {
        icaoType        = jog_s     (pSub, "icao");
        wake.wingSpan_m = float(jog_n_nan (pSub, "wingSpan"));
        wake.wingArea_m2= float(jog_n_nan (pSub, "wingArea"));
    }

    // attitude
    if ((pSub = json_object_get_object(obj, "attitude"))) {
        roll            = float(jog_n_nan (pSub, "roll"));
        heading         = float(jog_n_nan (pSub, "heading"));
        pitch           = float(jog_n_nan (pSub, "pitch"));
    }
    
    // config
    if ((pSub = json_object_get_object(obj, "config"))) {
        wake.mass_kg    = float(jog_n_nan (pSub, "mass"));
        wake.lift       = float(jog_n_nan (pSub, "lift"));
        gear            = float(jog_n_nan (pSub, "gear"));
        nws             = float(jog_n_nan (pSub, "noseWheel"));
        flaps           = float(jog_n_nan (pSub, "flaps"));
        spoilers        = float(jog_n_nan (pSub, "spoiler"));
    }

    // light
    if ((pSub = json_object_get_object(obj, "light"))) {
        lights.defined = true;
        lights.taxi    = jog_b(pSub, "taxi");
        lights.landing = jog_b(pSub, "landing");
        lights.beacon  = jog_b(pSub, "beacon");
        lights.strobe  = jog_b(pSub, "strobe");
        lights.nav     = jog_b(pSub, "nav");
    }

    return true;
}
