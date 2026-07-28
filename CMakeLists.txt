cmake_minimum_required(VERSION 3.16)
project(dsp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# raylib  (vendored side-by-side; already cloned into vendor/raylib)
# ---------------------------------------------------------------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # don't build raylib's examples
add_subdirectory(vendor/raylib)               # provides the `raylib` target
# Alternative (self-contained, no vendoring) — comment out the two lines above
# and uncomment this block to have CMake fetch raylib for you:
#   include(FetchContent)
#   FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git
#                               GIT_TAG 5.5)
#   set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#   FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# rtl-sdr  (build the 6 core files as a static lib linked into the app)
# ---------------------------------------------------------------------------
set(RTLSDR_DIR ${CMAKE_SOURCE_DIR}/vendor/rtl-sdr)
add_library(rtlsdr STATIC
    ${RTLSDR_DIR}/src/librtlsdr.c
    ${RTLSDR_DIR}/src/tuner_r82xx.c
    ${RTLSDR_DIR}/src/tuner_e4k.c
    ${RTLSDR_DIR}/src/tuner_fc0012.c
    ${RTLSDR_DIR}/src/tuner_fc0013.c
    ${RTLSDR_DIR}/src/tuner_fc2580.c
)
# rtlsdr_STATIC stops the header declaring the API as dllimport (we link it IN).
# PUBLIC so anything linking rtlsdr also sees the define + the include dir.
target_compile_definitions(rtlsdr PUBLIC rtlsdr_STATIC)
target_include_directories(rtlsdr PUBLIC ${RTLSDR_DIR}/include)

# ---------------------------------------------------------------------------
# libusb  (rtl-sdr's only external dependency)
#   Unix/macOS : found via pkg-config (system package)
#   Windows    : found via vcpkg (see vcpkg.json) or a manual find_library
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0)
endif()

if(LIBUSB_FOUND)
    target_include_directories(rtlsdr PRIVATE ${LIBUSB_INCLUDE_DIRS})
    target_link_libraries(rtlsdr PRIVATE ${LIBUSB_LINK_LIBRARIES})
else()
    find_path(LIBUSB_INCLUDE_DIR libusb.h PATH_SUFFIXES libusb-1.0)
    find_library(LIBUSB_LIBRARY NAMES usb-1.0 libusb-1.0)
    if(NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
        message(FATAL_ERROR
            "libusb not found. On Windows configure with the vcpkg toolchain "
            "(-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake); the bundled vcpkg.json "
            "will install it. On Linux/macOS install libusb-1.0 dev headers.")
    endif()
    target_include_directories(rtlsdr PRIVATE ${LIBUSB_INCLUDE_DIR})
    target_link_libraries(rtlsdr PRIVATE ${LIBUSB_LIBRARY})
endif()

# ---------------------------------------------------------------------------
# the app  (all .c files next to this CMakeLists)
# ---------------------------------------------------------------------------
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/*.c)
add_executable(dsp ${APP_SOURCES})
target_link_libraries(dsp PRIVATE raylib rtlsdr)

# Windowed app with no console on Windows, matching your old build.bat.
# Comment this out if you want a console for printf/debugging.
#if(WIN32 AND MSVC)
#    set_target_properties(dsp PROPERTIES
#        WIN32_EXECUTABLE TRUE
#        LINK_FLAGS "/ENTRY:mainCRTStartup")
#endif()

# On Windows + vcpkg, libusb-1.0.dll is copied next to the exe automatically
# (VCPKG_APPLOCAL_DEPS). Nothing to ship manually.