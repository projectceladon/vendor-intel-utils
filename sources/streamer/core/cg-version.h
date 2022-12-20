/**
 * @file   cg-version.h
 *
 * @brief  Cloud gaming version informations
 *
 *
 */

#ifndef __CG_VERSION_H__
#define __CG_VERSION_H__

#ifndef RC_INVOKED
#include <minwindef.h>
#endif

#define STR_VALUE(arg)  #arg
#define VER_VALUE(val)  STR_VALUE(val)

// CG major and minor version update manually
#define CG_MAJOR        01
#define CG_MINOR        02

// CG build number is from build system
#if (!defined(CG_BUILDNUMBER) || (~(~CG_BUILDNUMBER + 0) == 0 && ~(~CG_BUILDNUMBER + 1) == 1))
#undef CG_BUILDNUMBER
#define CG_BUILDNUMBER  0000
#endif

// CG revision
#ifndef CG_REVISION
#define CG_REVISION     00
#endif

// CG build model is update by build system (ci or verify) or local build
#if !defined(CG_BUILDMODEL)
#define CG_BUILDMODEL   "local"
#endif

// Resource informations
#define CG_COPYRIGHT         "Copyright \xA9 2022"
#define CG_MANUFACTURER      "Intel Corporation"
#define CG_LEGALCOPYRIGHT    CG_COPYRIGHT " " CG_MANUFACTURER
#define CG_PRODUCTNAME       "Cloud Gaming Reference Stack for Windows"

// File version and product version use in VERSIONINFO resource
#define CG_FILEVERSION          CG_MAJOR,CG_MINOR,CG_BUILDNUMBER,CG_REVISION
#define CG_FILEVERSION_STR      VER_VALUE(CG_MAJOR) "." VER_VALUE(CG_MINOR) "." VER_VALUE(CG_BUILDNUMBER) "." VER_VALUE(CG_REVISION)
#define CG_PRODUCTVERSION       CG_MAJOR,CG_MINOR,CG_BUILDNUMBER,CG_REVISION
#define CG_PRODUCTVERSION_STR   VER_VALUE(CG_MAJOR) "." VER_VALUE(CG_MINOR) "." VER_VALUE(CG_BUILDNUMBER) "." VER_VALUE(CG_REVISION)

// CG build platform (Win64 or Win32)
#if defined(_WIN32)
    #if defined(_WIN64)
        #define CG_PLATFORM	"Win64"
    #else
        #define CG_PLATFORM "Win32"
    #endif
#else
    #define CG_PLATFORM "Others"
#endif  // defined(_WIN32)

// CG build type (Release or Debug)
#if defined(_DEBUG)
    #define CG_BUILDTYPE "Debug"
#else
    #define CG_BUILDTYPE "Release"
#endif  // defined(DEBUG)

// CG build based on git commit id. In MSBuildAll.bat script will get branch's commit id.
#ifndef CG_GIT_COMMIT
#define CG_GIT_COMMIT "no_commit_info"
#endif

#ifndef RC_INVOKED
    #define CG_VERSION_STR  "cloud-gaming-" CG_BUILDMODEL "-" VER_VALUE(CG_BUILDNUMBER) "-" CG_BUILDTYPE "" CG_PLATFORM "-" CG_GIT_COMMIT
    static const char* CG_VERSION = CG_VERSION_STR;
#else
    // Macros for RC
    #define VERSION_BUILD   VER_VALUE(CG_BUILDMODEL) "-" VER_VALUE(CG_BUILDNUMBER) "-" CG_BUILDTYPE "" CG_PLATFORM "-" VER_VALUE(CG_GIT_COMMIT)
#endif

#endif  // __CG_VERSION_H__
