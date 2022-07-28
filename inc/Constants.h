/// @file       Constants.h
/// @brief      XPPlanes constant definitions
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

#define XPPLANES "XPPlanes"

// Helpers for converting barometric altitude (approx) to geo altitude
constexpr double HPA_STANDARD   = 1013.25;      ///< standard air pressure
/// @brief The pressure drops approximately by 11.3 Pa per meter in first 1000 meters above sea level.
/// @see https://en.wikipedia.org/wiki/Barometric_formula
constexpr double PA_per_M       = 11.3;
/// ft altitude diff per hPa change
constexpr double FT_per_HPA     = (100/PA_per_M)/XPMP2::M_per_FT;

/// Minimum time expected between two position to allow for meaningful interpolation
constexpr auto MIN_TS_DIFF = std::chrono::milliseconds(100);

/// Maximum `f` factor for non-location values during interpolation, like attitude, config
constexpr float MAX_F = 1.25f;

/// How long does the moment of touch down last? [seconds]
constexpr float TOUCH_DOWN_TIME = 0.5f;
