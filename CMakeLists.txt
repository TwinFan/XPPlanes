# XPPlanes - Display additional planes in XP based on network input

cmake_minimum_required(VERSION 3.16)
project(XPPlanes
        VERSION 0.2.0
        DESCRIPTION "XPPlanes X-Plane plugin")

# Provide compile macros from the above project version definition
add_compile_definitions(
    XPPLANES_VERSION_BETA=1
    XPPLANES_VER_MAJOR=${PROJECT_VERSION_MAJOR}
    XPPLANES_VER_MINOR=${PROJECT_VERSION_MINOR}
    XPPLANES_VER_PATCH=${PROJECT_VERSION_PATCH}
)

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
    add_compile_options(-mmacosx-version-min=10.12)
    add_link_options(-mmacosx-version-min=10.12)
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
if (MSVC)
    # Deprecation warning: once is enough
    add_compile_options(/wo4996)
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
        add_compile_options(-O0 -g)
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
include_directories("${CMAKE_SOURCE_DIR}/lib/XPMP2/XPMP2.framework/Versions/2.4/Headers")
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
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2")
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDK/Libraries/Mac")
elseif (UNIX)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/$ENV{platform}")
elseif (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/CURL")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/XPMP2/win")
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDK/Libraries/Win")
endif ()

# Link OpenGL related libraries.
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
include_directories( ${OpenGL_INCLUDE_DIRS} )
target_link_libraries( XPPlanes ${OpenGL_LIBRARIES} )

# Link the XPMP2 library
find_library(XPMP2_LIBRARY XPMP2 REQUIRED)
message ("   XPMP2_LIBRARY = ${XPMP2_LIBRARY}")
target_link_libraries(XPPlanes ${XPMP2_LIBRARY})

# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
    # XPLM is needed for any X-Plane plugin
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib REQUIRED)
    message ("   XPLM_LIBRARY  = ${XPLM_LIBRARY}")
    target_link_libraries(XPPlanes ${XPLM_LIBRARY} )
endif ()

if (WIN32)
    # Link platform-specific libraries especially for networking
    target_link_libraries(XPPlanes ws2_32.lib iphlpapi wldap32.lib advapi32.lib crypt32.lib)
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
