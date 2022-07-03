/// @file       Utilities.h
/// @brief      Miscellaneous utility functions, including logging
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
// MARK: General texts
//

#define ERR_ASSERT              "ASSERT FAILED: %s"
#define ERR_EXCEPTION           "EXCEPTION CAUGHT: %s"

//
// MARK: Misc
//

/// Get synched network time from X-Plane (sim/network/misc/network_time_sec) as used in Log.txt
float GetMiscNetwTime ();

/// @brief Convenience function to check on something at most every x seconds
/// @param _lastCheck Provide a float which holds the time of last check (init with `0.0f`)
/// @param _interval [seconds] How often to perform the check?
/// @param _now Current time, possibly from a call to GetTotalRunningTime()
/// @return `true` if more than `_interval` time has passed since `_lastCheck`
bool CheckEverySoOften (float& _lastCheck, float _interval, float _now);

/// @brief Convenience function to check on something at most every x seconds
/// @param _lastCheck Provide a float which holds the time of last check (init with `0.0f`)
/// @param _interval [seconds] How often to perform the check?
/// @return `true` if more than `_interval` time has passed since `_lastCheck`
inline bool CheckEverySoOften (float& _lastCheck, float _interval)
{ return CheckEverySoOften(_lastCheck, _interval, GetMiscNetwTime()); }

/// Return a plugin's name
std::string GetPluginName (XPLMPluginID who);

/// Read a text line, handling both Windows (CRLF) and Unix (LF) ending
std::istream& safeGetline(std::istream& is, std::string& t);

/// Compute geometric altitude [ft] from pressure altitude and current weather in a very simplistic manner good enough for the first 3,000ft
inline double WeatherAltCorr_ft (double pressureAlt_ft, double hPa)
{ return pressureAlt_ft + ((hPa - HPA_STANDARD) * FT_per_HPA); }

/// Replace NAN with 0.0
template<typename T>
inline T NZ (T v)
{ return std::isnan(v) ? T(0) : v; }

//
// MARK: String functions
//

/// @brief Copy _at most_ `n` chars from location, or less if zero-terminated.
/// @details Unlike std::string(char*, n) this will _not_ copy null chars
std::string str_n (const char* s, size_t max);

/// Simplify using str_n() with char arrays
#define STR_N(s) str_n(s,sizeof(s))

/// separates string into tokens
std::vector<std::string> str_tokenize (const std::string& s,
                                       const std::string& tokens,
                                       bool bSkipEmpty = true);

/// Class to extract tokens from a string
class StrTokens {
protected:
    const std::string& s;               ///< reference to the string to search
    std::string sep;                    ///< separators
    size_t p = std::string::npos;       ///< points to last separator found
    int num = 0;                        ///< counts the findings
public:
    /// Constructor: Pass in the string to separate and the separators
    StrTokens (const std::string& _s, const std::string& _sep) : s(_s), sep(_sep) {}
    /// returns the next token, can be the empty string if two tokens follow immediately, or if `finished()`
    std::string next();
    /// (re)sets the separators, then returns the next token
    std::string next(const std::string& _sep) { sep = _sep; return next(); }
    /// returns how many finds have been returned so far
    int count() const { return num; }
    /// have all tokens been returned?
    bool finished () const { return p != std::string::npos && p >= s.length(); }
};

/// Split the string at the first of the tokens and return the two pieces
std::pair<std::string,std::string> str_split (const std::string& s,
                                              const std::string& tokens);

//
// MARK: Logging Support
//

/// @brief To apply printf-style warnings to our functions.
/// @see Taken from imgui.h's definition of IM_FMTARGS
#if defined(__clang__) || defined(__GNUC__)
#define XPPLANES_FMTARGS(FMT)  __attribute__((format(printf, FMT, FMT+1)))
#else
#define XPPLANES_FMTARGS(FMT)
#endif

/// Logging level
enum logLevelTy : int {
    logDEBUG = 0,       ///< Debug, highest level of detail
    logINFO,            ///< regular info messages
    logWARN,            ///< warnings, i.e. unexpected, but uncritical events, maybe leading to unwanted display, but still: display of aircraft
    logERR,             ///< errors mean, aircraft can potentially not be displayed
    logFATAL,           ///< fatal is shortly before a crash
    logMSG              ///< will always be output, no matter what has been configured, cannot be suppressed
};

/// Returns ptr to static buffer filled with formatted log string
const char* LogGetString ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, va_list args );
             
/// Log Text to log file
void LogMsg ( const char* szFile, int ln, const char* szFunc, logLevelTy lvl, const char* szMsg, ... ) XPPLANES_FMTARGS(5);

//
// MARK: Logging macros
//

/// @brief Log a message if lvl is greater or equal currently defined log level
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define LOG_MSG(lvl,...)  {                                         \
    if (lvl >= glob.logLvl)                                         \
    {LogMsg(__FILE__, __LINE__, __func__, lvl, __VA_ARGS__);}       \
}

/// @brief Throws an exception using XPMP2Error
/// @note First parameter after lvl must be the message text,
///       which can be a format string with its parameters following like in sprintf
#define THROW_ERROR(...)                                            \
throw XPMP2::XPMP2Error(__FILE__, __LINE__, __func__, __VA_ARGS__);

/// @brief Throw in an assert-style (logging takes place in XPMP2Error constructor)
/// @note This conditional check _always_ takes place, independend of any build or logging settings!
#define LOG_ASSERT(cond)                                            \
    if (!(cond)) {                                                  \
        THROW_ERROR(ERR_ASSERT,#cond);                              \
    }

//
// MARK: Compiler differences
//

// XCode/Linux don't provide the _s functions, not even with __STDC_WANT_LIB_EXT1__ 1
#if APL
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ return strerror_r(errnum, buf, bufsz); }
#elif LIN
inline int strerror_s( char *buf, size_t bufsz, int errnum )
{ strerror_r(errnum, buf, bufsz); return 0; }
#endif

// MARK: Thread names
#ifdef DEBUG
// This might not work on older Windows version, which is why we don't publish it in release builds
#if IBM
#define SET_THREAD_NAME(sName) SetThreadDescription(GetCurrentThread(), L##sName)
#elif APL
#define SET_THREAD_NAME(sName) pthread_setname_np(sName)
#elif LIN
#define SET_THREAD_NAME(sName) pthread_setname_np(pthread_self(),sName)
#endif
#else
#define SET_THREAD_NAME(sName)
#endif
