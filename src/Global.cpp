/// @file       Global.cpp
/// @brief      Implements the GlobVars object holding all global variable information
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

/// The one global object for global variables
GlobVars glob;

//
// MARK: Config info definition
//

/// Defines an entry in the configuration file
class CfgInfoTy {
public:
    const char* tag = nullptr;          ///< name of the config entry in the config file
    int* pInt = nullptr;                ///< points to integer variable taking the value
    bool* pBool = nullptr;              ///< points to boolean variable taking the value
    std::string* pStr = nullptr;        ///< points to string variable taking the value

public:
    // Constructors
    CfgInfoTy (const char* _tag, int* pv) : tag(_tag), pInt(pv) {}
    CfgInfoTy (const char* _tag, int& v) : tag(_tag), pInt(&v) {}
    CfgInfoTy (const char* _tag, bool& v) : tag(_tag), pBool(&v) {}
    CfgInfoTy (const char* _tag, std::string& v) : tag(_tag), pStr(&v) {}

    /// Load a given config value into the right variable using the right type
    void LoadVal (const std::string& val)
    {
        if (pInt)
            *pInt = std::stoi(val);
        else if (pBool)
            *pBool = std::stoi(val) != 0;
        else if (pStr)
            *pStr = val;
        else {
            LOG_MSG(logERR, "No receiving variable defined for tag '%s'", tag);
        }
    }
    
    /// Return a full config string to be stored into the config file, including tag and CR
    std::string GetCfgLine () const
    {
        char s[256];
        if (pInt)
            snprintf(s, sizeof(s), "%s %d\n", tag, *pInt);
        else if (pBool)
            snprintf(s, sizeof(s), "%s %d\n", tag, *pBool);
        else if (pStr)
            snprintf(s, sizeof(s), "%s %s\n", tag, pStr->c_str());
        else {
            s[0] = 0;
            LOG_MSG(logERR, "No variable defined for tag '%s'", tag);
        }
        return std::string(s);
    }
};

/// Definition of all configuration values
static CfgInfoTy CFGINFO[] = {
    { "LogLevel",               (int*)&glob.logLvl              },
    { "LogModelMatch",          glob.bLogMdlMatch               },
    { "ObjReplDataRefs",        glob.bObjReplDataRefs           },
    { "ObjReplTextures",        glob.bObjReplTextures           },
    { "TCAS_Control",           glob.bAITcasControl             },
    { "PlanesMaxDist",          glob.maxPlaneDist               },
    { "PlanesClampAll",         glob.bClampAll                  },
    { "LabelsDraw",             glob.bDrawLabels                },
    { "LabelsMaxDist",          glob.maxLabelDist               },
    { "LabelsCutMaxVisible",    glob.bLabelCutOffAtVisibility   },
    { "MapEnable",              glob.bMapEnabled                },
    { "MapLabels",              glob.bMapLabels                 },
    { "NetMCGroup",             glob.remoteMCGroup              },
    { "NetListenPort",          glob.remotePort                 },
    { "NetTTL",                 glob.remoteTTL                  },
    { "NetBufSize",             glob.remoteBufSize              },
};

//
// MARK: Config File
//

/// Path to config file, relative to X-Plane
static const char* CFG_FILE_NAME = "Output/preferences/" XPPLANES ".prf";
/// Chars that are allowed as separator in the config file
static const char* CFG_TOKENS = " =\t";
constexpr size_t SERR_LEN = 1024;

// Read from a config file
bool GlobVars::ConfigFileLoad ()
{
    // open a config file
    std::ifstream fIn (CFG_FILE_NAME);
    if (!fIn) {
        // if there is no config file just return...that's no problem, we use defaults
        if (errno == ENOENT)
            return true;
        
        // something else happened
        char sErr[SERR_LEN];
        strerror_s(sErr, sizeof(sErr), errno);
        LOG_MSG(logERR, "Could not open config file '%s': %s",
                CFG_FILE_NAME, sErr);
        return false;
    }
    
    // First line shall be the version number
    std::string lnBuf;
    safeGetline(fIn, lnBuf);
    std::pair<std::string,std::string> val = str_split(lnBuf, CFG_TOKENS);
    if (!fIn ||
        val.first != XPPLANES ||                        // tag must be XPPlanes
        val.second.empty())                             // and there must be a version number
    {
        LOG_MSG(logERR, "Config file '%s' first line: Unsupported format or version: %s",
                CFG_FILE_NAME, lnBuf.c_str());
        return false;
    }
    [[maybe_unused]] const std::string cfgVer = val.second;
    
    // Read all other lines and interpret them
    while (fIn) {
        safeGetline(fIn, lnBuf);                        // read line and break into tokens, delimited by spaces
        if (lnBuf.empty()) continue;                    // skip empty lines without warning
        val = str_split(lnBuf, CFG_TOKENS);             // split into tag and value
        if (val.first.empty() || val.second.empty()) {  // didn't split into two words?
            LOG_MSG(logWARN, "Skipped invalid line '%s' in config file '%s'",
                    lnBuf.c_str(), CFG_FILE_NAME);
            continue;;
        }
        
        // Find a matching config element
        auto iter = std::find_if(std::begin(CFGINFO), std::end(CFGINFO),
                                 [&](const CfgInfoTy& o){return o.tag == val.first;});
        if (iter != std::end(CFGINFO))                  // found a config value in our list
            iter->LoadVal(val.second);                  // load the value
        else {
            LOG_MSG(logWARN, "Skipped unknown config value '%s'", lnBuf.c_str());
        }
    }

    
    // Close and return
    fIn.close();
    return true;
}

// Write to a config file
bool GlobVars::ConfigFileSave ()
{
    // open an output config file
    std::ofstream fOut (CFG_FILE_NAME, std::ios_base::out | std::ios_base::trunc);
    if (!fOut) {
        char sErr[SERR_LEN];
        strerror_s(sErr, sizeof(sErr), errno);
        LOG_MSG(logERR, "Could not create config file '%s': %s",
                CFG_FILE_NAME, sErr);
        return false;
    }

    // save application and version first
    fOut << XPPLANES << ' ' << XPPLANES_VER_MAJOR << '.' << XPPLANES_VER_MINOR << '.' << XPPLANES_VER_PATCH << '\n';
    
    // Save all config values
    for (const CfgInfoTy& cfg: CFGINFO)
        fOut << cfg.GetCfgLine();
    
    // Close and return
    fOut.flush();
    fOut.close();
    return true;
}
