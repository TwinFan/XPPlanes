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
    /// @brief Constructor creates a new aircraft object, which will be managed and displayed
    /// @exception XPMP2::XPMP2Error Mode S id invalid or duplicate, no model found during model matching
    /// @param _icaoType ICAO aircraft type designator, like 'A320', 'B738', 'C172'
    /// @param _icaoAirline ICAO airline code, like 'BAW', 'DLH', can be an empty string
    /// @param _livery Special livery designator, can be an empty string
    /// @param _modeS_id (optional) **Unique** identification of the plane [0x01..0xFFFFFF], e.g. the 24bit mode S transponder code. XPMP2 assigns an arbitrary unique number of not given
    /// @param _cslId (optional) specific unique model id to be used (package name/short id, as defined in the `OBJ8_AIRCRAFT` line)
    Plane (const std::string& _icaoType,
           const std::string& _icaoAirline,
           const std::string& _livery,
           XPMPPlaneID _modeS_id = 0,
           const std::string& _cslId = "");
    
    /// Destructor cleans up all resources acquired
    ~Plane() override;
    
    /// Called by XPMP2 right before updating the aircraft's placement in the world
    void UpdatePosition (float _elapsedSinceLastCall, int _flCounter) override;

protected:
    /// Is this plane to be removed next flight loop?
    bool    bToBeRemoved = false;
    
public:
    /// @brief Determines if this plane shall be removed, sets `bToBeRemoved` if so
    /// @details Reasons for removal:
    ///          - no updates for too long
    ///          - too far away from camera
    bool ComputeToBeRemoved ();
    /// Shall this plane be removed?
    bool ShallBeRemoved () const { return bToBeRemoved; }
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
