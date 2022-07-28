/// @file       Plane.cpp
/// @brief      The Plane class represents, guess what, a plane
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

//
// MARK: Process Flight Data
//

void PlaneMaintenance ()
{
    // *** Update from FlightData lists***
    tsTy now = std::chrono::system_clock::now();
    tsTy cutOff = now - std::chrono::seconds(glob.gracePeriod);
    
    // Loop over map/list of flight data and see if we need to create or update planes
    bool bHaveData = false;
    {
        std::lock_guard<std::mutex> guard(glob.mtxListFD);          // guarded by a mutex so that network thread doesn't update

        bHaveData = !glob.mapListFD.empty();
        if (glob.eStatus == GlobVars::STATUS_WAITING && bHaveData) {// if there is data we are no longer 'waiting'
            glob.eStatus = GlobVars::STATUS_ACTIVE;
            LOG_MSG(logINFO, "Status turned ACTIVE");
        }

        for (auto iPlaneFD = glob.mapListFD.begin();
             iPlaneFD != glob.mapListFD.end();)
        {
            // Remove outdated data from the list just to make sure we clean up properly
            listFlightDataTy& listFd = iPlaneFD->second;
            while (!listFd.empty() && listFd.front()->ts < cutOff)
                listFd.pop_front();
            
            // if there is no data then remove the plane's entry
            if (iPlaneFD->second.empty()) {
                iPlaneFD = glob.mapListFD.erase(iPlaneFD);
                continue;
            }
            
            // Is there already a matching plane?
            try {
                Plane& plane = glob.mapPlanes.at(iPlaneFD->first);
                plane.UpdateFromFlightData(iPlaneFD->second, now);
            }
            catch (const std::out_of_range&) {
                // there is no such plane yet, do we have enough data to create one?
                if (iPlaneFD->second.size() >= 2) {
                    // fetch the two starting position from the list
                    ptrFlightDataTy from = std::move(iPlaneFD->second.front());
                    iPlaneFD->second.pop_front();
                    ptrFlightDataTy to = std::move(iPlaneFD->second.front());
                    iPlaneFD->second.pop_front();
                    // and create a plane with those
                    glob.mapPlanes.emplace(std::piecewise_construct,
                                           std::forward_as_tuple(iPlaneFD->first),
                                           std::forward_as_tuple(std::move(from), std::move(to)));
                }
            }
            
            // next entry
            ++iPlaneFD;
        }
    }
    
    // *** Remove planes that say so ***
    now -= std::chrono::seconds(glob.gracePeriod);       // deduct grace period
    for (auto iPlane = glob.mapPlanes.begin();
         iPlane != glob.mapPlanes.end();)
    {
        if (glob.eStatus == GlobVars::STATUS_INACTIVE ||    // remove all planes if (turned) inactive
            iPlane->second.ShallBeRemoved(now))             // or if plane itself says so
            iPlane = glob.mapPlanes.erase(iPlane);
        else
            ++iPlane;
    }
    
    // if there are neither planes nor data we should probably be waiting
    if (glob.eStatus == GlobVars::STATUS_ACTIVE &&
        !bHaveData &&
        glob.mapPlanes.empty())
    {
        glob.eStatus = GlobVars::STATUS_WAITING;
        LOG_MSG(logINFO, "Status turned back to WAITING");
    }
    
}

// Regularly called to update from/to positions from the list of available flight data
void Plane::UpdateFromFlightData (listFlightDataTy& listFD,
                                  const tsTy& now)
{
    // Younger data needs to have a ts larger than this CutOff time
    const auto tsCutOff = fdTo->ts + MIN_TS_DIFF;
    // Loop all flight data (sorted), from the oldest to the newest:
    for (auto iFD = listFD.begin();
         iFD != listFD.end();)
    {
        // Cleanup: remove all flight data from the list that is useless because it
        // is already older than my current 'to' position:
        if (iFD->get()->ts <= tsCutOff)
            iFD = listFD.erase(iFD);
        else
        {
            // So iFD points to the first FlightData that is younger than fdTo
            // If fdTo is still in the future, then we don't yet need that data and are done
            if (fdTo->ts > now)
                break;
            
            // Otherwise we need that new data iFD points to
            
            // Shift current 'to' to 'from'
            fdFrom = std::move(fdTo);
            diFrom = diTo;
            
            // get fresh 'to' from the flight data list
            TakeOverData(false, std::move(*iFD));
            
            // So far, we only established that the new 'to' is younger than 'from',
            // but if the new 'to' is actually in the future compared to 'now',
            // then we can move from our _current_ position to that new position,
            // that avoids any sudden jumping of the plane
            if (fdTo->ts > now) {
                diFrom = drawInfo;                  // current position
                XPLMLocalToWorld(diFrom.x, diFrom.y, diFrom.y,
                                 &fdFrom->lat, &fdFrom->lon, &fdFrom->alt_m);
                fdFrom->ts = now;                   // as of right now
            }
            
            // Remove the object from the list
            // and continue in the loop...maybe that just added data is already outdated...?
            iFD = listFD.erase(iFD);
        }
    }
}

// Prepare given position for usage after taking over from passed-in smart pointer
void Plane::TakeOverData (bool bFrom, ptrFlightDataTy&& source)
{
    // Which members to act on?
    ptrFlightDataTy& fd = bFrom ? fdFrom : fdTo;
    XPLMDrawInfo_t&  di = bFrom ? diFrom : diTo;
    
    fd = std::move(source);                     // move the smart pointer
    if (fd->bGnd) DetermineGndAlt(fd);          // if on gnd let's figure out where gnd is
    if (bFrom)                                  // In a `from` case (only once from the constructor)
        fd->NANtoZero();                        // we set all `NAN`s to zero so we have a start basis to draw the plane
    else                                        // In all future updates we keep the current value stable if no new value arrives
        fd->NANtoCopy(*fdFrom);                 // by copying from `from` to `to`
    di = *fd;                                   // convert to XP's draw info
    di.y += GetVertOfs();                       // vertical offset to make plane move on wheels
    
    // Test for a change in model-defining data, need a new CSL model match?
    if (!bFrom &&
        (fd->icaoType       != acIcaoType       ||
         fd->icaoAirline    != acIcaoAirline    ||
         fd->livery         != acLivery))
    {
        ChangeModel(fd->icaoType, fd->icaoAirline, fd->livery);
    }
    
    // Calculate the aircraft label
    if (!fd->label.empty()) {                   // label is passed in
        label = fd->label;
    }
    else
    {
        if (fd->callSign.empty()) {             // Id is callsign or hex id
            char sId[10];
            snprintf(sId, sizeof(sId), "0x%06X", modeS_id);
            label = sId;
        } else
            label = fd->callSign;

        if (!fd->icaoType.empty()) {            // Add a/c type
            label += " (";
            label += fd->icaoType;
            label += ')';
        }
    }
}



// Clamp to ground: Make sure the plane is not below ground, corrects Aircraft::drawInfo if needed.
void Plane::DetermineGndAlt (ptrFlightDataTy& fd)
{
    // Make sure we have a probe object (an attribute of XPMP2::Aircraft and cleaned up by its destructor)
    if (!hProbe)
        hProbe = XPLMCreateProbe(xplm_ProbeY);
    LOG_ASSERT(hProbe);
    
    // Convert lat/lon to OpenGL
    double X = 0.0, Y = 0.0, Z = 0.0;
    XPLMWorldToLocal(fd->lat, fd->lon, 0.0,
                     &X, &Y, &Z);
    
    // Where's the ground?
    XPLMProbeInfo_t infoProbe = {
        sizeof(XPLMProbeInfo_t),            // structSIze
        0.0f, 0.0f, 0.0f,                   // location
        0.0f, 0.0f, 0.0f,                   // normal vector
        0.0f, 0.0f, 0.0f,                   // velocity vector
        0                                   // is_wet
    };
    if (XPLMProbeTerrainXYZ(hProbe,
                            float(X), float(Y), float(Z),
                            &infoProbe) == xplm_ProbeHitTerrain)
    {
        // Return the altitude back to world coordinates
        XPLMLocalToWorld(double(infoProbe.locationX),
                         double(infoProbe.locationY),
                         double(infoProbe.locationZ),
                         &X, &Y, &fd->alt_m);
    }
    else {
        // probe failed...so we need to assume something
        fd->alt_m = 0.0;
    }
}
// Should this plane be removed?
bool Plane::ShallBeRemoved (const tsTy& cutOff) const
{
    return
    // not updated for too long?
    fdTo->ts < cutOff;
}

//
// MARK: Once per Cycle
//

// This data is updated once per cycle, then reused by other Update... calls
int Plane::flCounter = -1;          ///< flight loop counter of last update
tsTy::rep Plane::ticksNow = 0;      ///< 'now' timestamp

// Once per cycle activities
void Plane::OncePerCycle (int _flCounter)
{
    if (_flCounter <= flCounter) return;
    ticksNow = std::chrono::system_clock::now().time_since_epoch().count();
}

//
// MARK: XPMP2 Interface
//

// Constructor from two flight data objects
Plane::Plane (ptrFlightDataTy&& from, ptrFlightDataTy&& to) :
XPMP2::Aircraft(from->icaoType, from->icaoAirline, from->livery,
                from->_modeS_id)
{
    // Take over the flight data
    TakeOverData(true,  std::move(from));
    TakeOverData(false, std::move(to));
    
    // as we have take care of terrain already we don't need clamping
    bClampToGround = false;
}

// Destructor
Plane::~Plane ()
{}

/// @brief Calculate interpolation between 0 and 1 from the FlightData objects
/// @details If `to` is `NAN` then does nothing, does not change the existing value,
///          else if `from` is `NAN` sets just the `to` value,
///          otherwise interpolates between `from` and `to` using `f`
#define IP_01(fct,v)                                                            \
fct(std::clamp<float>(std::fmaf(f, fdTo->v - fdFrom->v, fdFrom->v), 0.0f,1.0f));
// fmaf(x,y,z) = x*y + z

// Called by XPMP2 right before updating the aircraft's placement in the world
void Plane::UpdatePosition (float /*_elapsedSinceLastCall*/, int _flCounter)
{
    try {
        // Once per cycle
        OncePerCycle(_flCounter);
        
        // Interpolation between fdFrom and fdTo
        const tsTy::rep tsFrom = fdFrom->ts.time_since_epoch().count();
        const tsTy::rep tsTo   = fdTo->ts.time_since_epoch().count();
        // _the_ factor: increases from 0 to 1 while `now` is between `from` and `to` (->interpolation),
        // and becomes larger than 1 if `now` increases even beyond `to` (-> extrapolation)
        f = float(ticksNow - tsFrom) / float(tsTo - tsFrom);
        LOG_ASSERT(!std::isnan(f));
        
        // Update the drawInfo with interpolated values
        // Location [ fmaf(x,y,z) = x*y + z ]
        drawInfo.x      = std::fmaf(f, diTo.x - diFrom.x, diFrom.x);
        drawInfo.y      = std::fmaf(f, diTo.y - diFrom.y, diFrom.y);
        drawInfo.z      = std::fmaf(f, diTo.z - diFrom.z, diFrom.z);
        
        // for all the following values we cap `f` at 1.25 so we don't do too much of spinning etc in case we are missing future updates
        if (f > MAX_F) f = MAX_F;
        
        // Attitude
        drawInfo.pitch  = std::fmaf(f, diTo.pitch - diFrom.pitch,              diFrom.pitch);
        drawInfo.roll   = std::fmaf(f, diTo.roll  - diFrom.roll,               diFrom.roll);
        drawInfo.heading= std::fmaf(f, HeadDiff(diFrom.heading, diTo.heading), diFrom.heading);
        
        // Configuration
        IP_01(SetGearRatio, gear);
        SetNoseWheelAngle(std::fmaf(f, HeadDiff(fdFrom->nws, fdTo->nws), fdFrom->nws));
        IP_01(SetFlapRatio, flaps);
        IP_01(SetSpoilerRatio, spoilers);
        
        // Lights
        const FlightData::lightsTy& lights = (f >= 0.5) ? fdTo->lights : fdFrom->lights;
        if (lights.defined) {
            SetLightsTaxi(lights.taxi);
            SetLightsLanding(lights.landing);
            SetLightsBeacon(lights.beacon);
            SetLightsStrobe(lights.strobe);
            SetLightsNav(lights.nav);
        }
    }
    catch (const std::exception& e) {
        LOG_MSG(logWARN, "Updating 0x%06X failed: %s", modeS_id, e.what());
    }
}


/// Lift produced. Either given in `wake.lift` or simple defaults apply
float Plane::GetLift() const
{
    // replace any missing value with default, then just interpolate
    if (std::isnan(fdFrom->wake.lift))
        fdFrom->wake.lift = XPMP2::Aircraft::GetLift();
    if (std::isnan(fdTo->wake.lift))
        fdTo->wake.lift = XPMP2::Aircraft::GetLift();

    // return interpolation between these two
    return std::fmaf(f, fdTo->wake.lift - fdFrom->wake.lift, fdFrom->wake.lift);
}


//
// MARK: Global Functions
//

/// Initialie the Plane module
bool PlaneStartup()
{
    // doesn't do anything at the moment
    return true;
}

/// Shutdown the plane module
void PlaneShutdown()
{
    // remove all planes
    glob.mapPlanes.clear();
}
