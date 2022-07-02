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
// MARK: XPMP2 Interface
//

// Constructor
Plane::Plane (const std::string& _icaoType,
              const std::string& _icaoAirline,
              const std::string& _livery,
              XPMPPlaneID _modeS_id,
              const std::string& _cslId) :
XPMP2::Aircraft(_icaoType, _icaoAirline, _livery, _modeS_id, _cslId)
{}

// Destructor
Plane::~Plane ()
{}

// Called by XPMP2 right before updating the aircraft's placement in the world
void Plane::UpdatePosition (float _elapsedSinceLastCall, int _flCounter)
{
    const tsTy now = std::chrono::steady_clock::now();
    
    
    // TODO: Implement UpdatePosition
}


//
// MARK: XPPlanes control
//

// Should this plane be removed?
bool Plane::ComputeToBeRemoved ()
{
    // Too far away from camera position?
    return bToBeRemoved = camDist > glob.maxPlaneDist * XPMP2::M_per_NM;
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
