/// @file       Plane.h
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

#pragma once

//
// MARK: Plane class
//

class Plane : public XPMP2::Aircraft
{
public:
    /// @brief Create a new plane from two flight data objects
    Plane (ptrFlightDataTy&& from, ptrFlightDataTy&& to);
    
    /// Destructor cleans up all resources acquired
    ~Plane() override;
    
    /// Called by XPMP2 right before updating the aircraft's placement in the world
    void UpdatePosition (float _elapsedSinceLastCall, int _flCounter) override;

    /// Lift produced. Either given in `wake.lift` or simple defaults apply
    float GetLift() const override;

protected:
    ptrFlightDataTy fdFrom;         ///< the from-position for interpolation
    ptrFlightDataTy fdTo;           ///< the to-position for interpolation
    XPLMDrawInfo_t  diFrom;         ///< the from-position in XP speak
    XPLMDrawInfo_t  diTo;           ///< the to-position in XP speak
    /// _The_ factor: increases from 0 to 1 while `now` is between `from` and `to` (->interpolation),
    /// and becomes larger than 1 if `now` increases even beyond `to` (-> extrapolation)
    float           f = 0.5f;
    XPLMProbeRef hProbe = NULL;     ///< probe reference needed to determine
    
    /// @brief Prepare given position for usage after taking over from passed-in smart pointer
    /// @param bFrom Store into `from` variables? Otherwise into `to`
    /// @param source From where to take over the data
    void TakeOverData (bool bFrom, ptrFlightDataTy&& source);
    
public:
    /// Regularly called to update from/to positions from the list of available flight data
    void UpdateFromFlightData (listFlightDataTy& listFD,
                               const tsTy& now);
    
    /// Determine ground altitude of a given location
    void DetermineGndAlt (ptrFlightDataTy& fd);
    
    /// @brief Determines if this plane shall be removed, sets `bToBeRemoved` if so
    /// @details Reasons for removal:
    ///          - no updates for too long
    ///          - too far away from camera
    bool ShallBeRemoved (const tsTy& cutOff) const;
    
    // This data is updated once per cycle, then reused by other Update... calls
protected:
    static int flCounter;           ///< flight loop counter of last update
    static tsTy::rep ticksNow;      ///< 'now' timestamp in ticks since epoch
    /// perform once-per-cycle activities
    static void OncePerCycle (int _flCounter);
};

/// Type of the map that stores and owns the plane objects
typedef std::map<XPMPPlaneID,Plane> mapPlanesTy;

//
// MARK: Global Functions
//

/// Initialie the Plane module
bool PlaneStartup();

/// Shutdown the plane module
void PlaneShutdown();

/// Regular updates from flight data
void PlaneMaintenance ();
