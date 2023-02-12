# XPPlanes - Display additional planes in XP based on network input
# - Set up to be used in the provided docker environment to build lin and mac
# - Set up to be used in a Visual Studio environment to build win (File > Open > Folder, then VS recognized the CMAKE configuration)
#
# If you want to build with FMOD sound support,
# 1. Make sure to understand the FMOD license and attribution requirements:
#    https://www.fmod.com/licensing
#    https://www.fmod.com/attribution
# 2. Define INCLUDE_FMOD_SOUND cache entry, e.g. using `cmake -G Ninja -D INCLUDE_FMOD_SOUND=1 ..`

cmake_minimum_required(VERSION 3.16)

# Mac: Need to tell early on that we want a cross platform build
if(DEFINED ENV{platform})
    message ("-- Platform is $ENV{platform}")
    if($ENV{platform} STREQUAL "mac-x86")
        message ("   Building cross-platform for mac/x86_64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Archs to build")
    elseif($ENV{platform} STREQUAL "mac-arm")
        message ("   Building cross-platform for mac/arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Archs to build")
    endif()
else()
    # No 'platform' defined could mean running from command line, assume we build universal image in one go via XCode
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Archs to build")
endif()

project(XPPlanes
        VERSION 0.3.0
        DESCRIPTION "XPPlanes X-Plane plugin")

# Provide compile macros from the above project version definition
add_compile_definitions(
    XPPLANES_VERSION_BETA=0
    XPPLANES_VER_MAJOR=${PROJECT_VERSION_MAJOR}
    XPPLANES_VER_MINOR=${PROJECT_VERSION_MINOR}
    XPPLANES_VER_PATCH=${PROJECT_VERSION_PATCH}
)

message ("== Building: ${PROJECT_NAME} ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} ==")
message ("Compiler Info:")
message ("CMAKE_CXX_COMPILER_ID         = ${CMAKE_CXX_COMPILER_ID}")
message ("CMAKE_CXX_COMPILER_VERSION    = ${CMAKE_CXX_COMPILER_VERSION}")
message ("CMAKE_CXX_COMPILER            = ${CMAKE_CXX_COMPILER}")
message ("WIN32 / MSVC / MINGW          = ${WIN32} / ${MSVC} / ${MINGW}")
message ("UNIX / APPLE                  = ${UNIX} / ${APPLE}")
if (APPLE)
    message ("OSX_SDK_PATH                  = $ENV{OSX_SDK_PATH}")
    message ("CMAKE_OSX_ARCHITECTURES       = ${CMAKE_OSX_ARCHITECTURES}")
endif()

################################################################################
# Target Systems
################################################################################

# Windows: Target Windows 7.0 and later
if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
    if (NOT DEFINED ENV{platform})
        set(ENV{platform} "win")
    endif()
elseif(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    add_compile_options(-mmacosx-version-min=11.0)
    add_link_options(-mmacosx-version-min=11.0)
endif()

################################################################################
# C++ Standard required
################################################################################

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED 17)
set_property(GLOBAL PROPERTY CXX_STANDARD 17)

################################################################################
# Compile Options
################################################################################

# Enable all X-Plane SDK APIs up to the newest version.
add_compile_definitions(XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM303=1)

# Define platform macros.
add_compile_definitions(APL=$<BOOL:${APPLE}> IBM=$<BOOL:${WIN32}> LIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

# Enable stricter warnings and then disable some we are not interested in.
# For XPMP2 compile, we don't need to be warned about our self-defined depreciations
if (MSVC)
    add_compile_options(/wd4996 /wd4068)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 9.0 AND NOT APPLE)
        add_compile_options(-Wno-stringop-truncation)
    endif()

    # Force-enable exception support. This is most likely redundant, although for C
    # code the default is the opposite. Since we are mixing C++ and C libraries,
    # safer to set it on?
    add_compile_options(-fexceptions)

    # Makes symbols non-exported by default.
    add_compile_options(-fvisibility=hidden)
endif()


# Debug vs Release build
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_definitions(DEBUG=1)
    if (MSVC)
        add_compile_options(/Zi)
    else()
        add_compile_options(-O0 -g -fPIC)
    endif()
else()
    add_compile_definitions(NDEBUG=1)
    if(MSVC)
        # Use highest optimization level in Release builds
        add_compile_options(/GL)
    elseif(APPLE)
        add_compile_options(-O3 -fPIC)
    elseif (UNIX OR MINGW)
        # Use position-independent code and highest optimization level (FPS!).
        add_compile_options(-O3 -fPIC)
        # Strip symbols during linking
        add_link_options(-s)
    endif()
endif()

# Mingw Threads
if (MINGW)
    option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
    add_subdirectory(lib/mingw-std-threads)
endif()

################################################################################
# Includes
################################################################################

# Set include directories used by our code and dependencies.
include_directories("${CMAKE_SOURCE_DIR}/inc")
include_directories("${CMAKE_SOURCE_DIR}/lib")
include_directories("${CMAKE_SOURCE_DIR}/lib/SDK/CHeaders/XPLM")
include_directories("${CMAKE_SOURCE_DIR}/lib/XPMP2/inc")
include_directories("${CMAKE_SOURCE_DIR}/lib/OpenGL")

################################################################################
# Source files
################################################################################
add_library(XPPlanes MODULE
    lib/OpenGL/SystemGL.h
    lib/parson/parson.h
    lib/parson/parsonWrapper.h
    inc/Constants.h
    inc/FlightData.h
    inc/Global.h
    inc/Listener.h
    inc/Network.h
    inc/Plane.h
    inc/Utilities.h
    inc/XPPlanes.h
    lib/parson/parson.c
    lib/parson/parsonWrapper.cpp
    src/FlightData.cpp
    src/FD_RTTFC.cpp
    src/FD_XPPTraffic.cpp
    src/Global.cpp
    src/Listener.cpp
    src/main.cpp
    src/Network.cpp
    src/Plane.cpp
    src/Utilities.cpp
)

################################################################################
# Define pre-compiled header
################################################################################

target_precompile_headers(XPPlanes PUBLIC inc/XPPlanes.h)

# Exclude all the non-core-LiveTraffic modules from the pch
set_source_files_properties(
    lib/parson/parson.c
    lib/parson/parsonWrapper.cpp
PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON)

  
################################################################################
# Link Libraries
################################################################################

# Specify library search locations.
if (APPLE)
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDK/Libraries/Mac")
elseif (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/CURL")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDK/Libraries/Win")
endif ()

# Link OpenGL related libraries.
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
include_directories( ${OpenGL_INCLUDE_DIRS} )
target_link_libraries( XPPlanes ${OpenGL_LIBRARIES} )

# FMOD Library only if requested
if(INCLUDE_FMOD_SOUND)
    # Compile options
    add_compile_definitions(INCLUDE_FMOD_SOUND)

    # Link Options (only for Win/Mac)
    if(WIN32 OR APPLE)
        list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/fmod")
        find_library(FMOD_LIBRARY NAMES fmod_vc.lib libfmod.dylib REQUIRED)
        message ("   FMOD_LIBRARY               = ${FMOD_LIBRARY}")
    endif()
endif()

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    # XPLM is needed for any X-Plane plugin
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib REQUIRED)
    message ("   XPLM_LIBRARY               = ${XPLM_LIBRARY}")
    target_link_libraries(XPPlanes ${XPLM_LIBRARY} )
endif ()

# Incude building/linking XPMP2
add_subdirectory(lib/XPMP2)
add_dependencies(XPPlanes XPMP2)
target_link_libraries(XPPlanes XPMP2)

if (WIN32)
    # Link platform-specific libraries especially for networking
    target_link_libraries(XPPlanes ${XPLM_LIBRARY} ${FMOD_LIBRARY} ws2_32.lib iphlpapi wldap32.lib advapi32.lib crypt32.lib)
    if (MINGW)
        # Include MingW threads
        target_link_libraries(XPPlanes mingw_stdthreads)
        # When cross-compiling we link the standard libraries statically
        target_link_options(XPPlanes PRIVATE -static-libgcc -static-libstdc++)
    endif()
elseif (APPLE)
    # Link OS X core system libraries.
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(Cocoa_LIBRARY Cocoa REQUIRED)
    find_library(Security_LIBRARY Security REQUIRED)
    find_library(GSS_LIBRARY GSS REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(XPPlanes
        ${CORE_FOUNDATION_LIBRARY}
        ${Cocoa_LIBRARY}
        ${Security_LIBRARY}
        ${GSS_LIBRARY}
        ${OpenGL_LIBRARY}
        ${FMOD_LIBRARY}
        ${XPLM_LIBRARY}  
    )

    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_libraries(XPPlanes "-exported_symbols_list ${CMAKE_SOURCE_DIR}/src/XPPlanes.sym_mac")
elseif (UNIX)
    # Link library for dynamic loading of shared objects on UNIX systems.
    find_library(DL_LIBRARY dl REQUIRED)
    # Threads
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
    target_link_libraries(XPPlanes ${DL_LIBRARY} Threads::Threads)
    # Specify additional runtime search paths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin to the ones required by XPLM:
    target_link_libraries(XPPlanes -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/src/XPPlanes.sym)
endif ()

# Target directory and file name
if (WIN32)
   set_target_properties(XPPlanes PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/win_x64")
elseif (APPLE)
   set_target_properties(XPPlanes PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mac_x64")
elseif (UNIX)
   set_target_properties(XPPlanes PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lin_x64")
endif ()

set_target_properties(XPPlanes
    PROPERTIES
    PREFIX ""
    OUTPUT_NAME "XPPlanes"
    SUFFIX ".xpl"
)
