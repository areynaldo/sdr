cmake_minimum_required(VERSION 3.16)
# Dear ImGui / rlImGui are C++, so the project must enable CXX even though our
# own code — main.c, sdr.c — stays pure C and calls the widgets via cimgui.
project(dsp C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# raylib  (vendored side-by-side; already cloned into vendor/raylib)
# ---------------------------------------------------------------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # don't build raylib's examples
add_subdirectory(vendor/raylib)               # provides the `raylib` target

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
target_compile_definitions(rtlsdr PUBLIC rtlsdr_STATIC)
target_include_directories(rtlsdr PUBLIC ${RTLSDR_DIR}/include)

# ---------------------------------------------------------------------------
# libusb  (rtl-sdr's only external dependency)
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

# ===========================================================================
# Dear ImGui UI: latest cimgui (C widget API) + rlImGui (C++ raylib backend)
#
#   Clone both at their latest — no pinning needed this time, because rlImGui
#   tracks current Dear ImGui (1.92+), unlike the unmaintained rlcimgui:
#
#     git clone --recursive https://github.com/cimgui/cimgui.git vendor/cimgui
#     cd vendor/cimgui && git checkout docking_inter \
#         && git submodule update --init --recursive && cd ../..
#     git clone https://github.com/raylib-extras/rlImGui.git vendor/rlImGui
#
#   THE KEY RULE: there is exactly ONE Dear ImGui in the whole program. It is
#   compiled once, inside the `cimgui` target below (cimgui bundles imgui as its
#   submodule). rlImGui does NOT compile its own imgui — it links `cimgui` and
#   includes those same headers. Both call into the same global ImGui context,
#   so this single-copy rule is what keeps them from corrupting each other.
# ===========================================================================
set(CIMGUI_DIR  ${CMAKE_SOURCE_DIR}/vendor/cimgui)
set(RLIMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/rlImGui)

# --- cimgui + the one-and-only Dear ImGui (compiled here, once) ------------
add_library(cimgui STATIC
    ${CIMGUI_DIR}/cimgui.cpp
    ${CIMGUI_DIR}/imgui/imgui.cpp
    ${CIMGUI_DIR}/imgui/imgui_draw.cpp
    ${CIMGUI_DIR}/imgui/imgui_tables.cpp
    ${CIMGUI_DIR}/imgui/imgui_widgets.cpp
    ${CIMGUI_DIR}/imgui/imgui_demo.cpp     # drop if you never call igShowDemoWindow()
)
set_target_properties(cimgui PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
target_include_directories(cimgui PUBLIC ${CIMGUI_DIR} ${CIMGUI_DIR}/imgui)
# PUBLIC — applies to cimgui.cpp AND every consumer, because these change ImGui
# struct LAYOUT and linkage; all TUs that see imgui.h/cimgui.h must agree.
#   * IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1 — matches how cimgui's bindings are
#     generated (mismatch here = silent memory corruption, not a compile error).
#   * CIMGUI_NO_EXPORT — static link on Windows (no dllimport/export tagging).
target_compile_definitions(cimgui PUBLIC
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    CIMGUI_NO_EXPORT
)
# INTERFACE — consumers ONLY, never cimgui.cpp itself. This makes cimgui.h emit
# real C bodies for every enum/struct. cimgui.cpp already gets those from the
# C++ imgui.h, so defining it there too would declare each enum twice — that is
# exactly the C2365/C2371 "redefinition" storm. INTERFACE keeps it off cimgui's
# own sources while still handing it to main.c (rlImGui.cpp is C++ and ignores
# it, since it includes imgui.h rather than cimgui.h).
target_compile_definitions(cimgui INTERFACE
    CIMGUI_DEFINE_ENUMS_AND_STRUCTS
)

# --- rlImGui backend: links the imgui above, never recompiles it -----------
add_library(rlimgui STATIC ${RLIMGUI_DIR}/rlImGui.cpp)
set_target_properties(rlimgui PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
target_include_directories(rlimgui PUBLIC ${RLIMGUI_DIR})
# Linking cimgui PUBLICLY gives rlImGui.cpp the imgui headers AND the exact same
# layout defines above — so it compiles against the very imgui it links.
target_link_libraries(rlimgui PUBLIC cimgui raylib)
# If Font Awesome ever gets in the way:
# target_compile_definitions(rlimgui PRIVATE NO_FONT_AWESOME)

# ---------------------------------------------------------------------------
# the app  (all .c files next to this CMakeLists)
# ---------------------------------------------------------------------------
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/main.c)
add_executable(dsp ${APP_SOURCES})
# Linking rlimgui transitively brings in cimgui, imgui, raylib, all the include
# dirs, and the layout defines — so main.c can just:
#     #include "cimgui.h"     // ig* widget calls
#     #include "rlImGui.h"    // rlImGuiSetup/Begin/End/Shutdown + image helpers
target_link_libraries(dsp PRIVATE raylib rtlsdr rlimgui)

# dsp has only C sources but links C++ objects, so force the final link through
# the C++ driver to pull in the C++ runtime (portable; no manual -lstdc++).
set_target_properties(dsp PROPERTIES LINKER_LANGUAGE CXX)

# Windowed app with no console on Windows (uncomment to match your build.bat):
#if(WIN32 AND MSVC)
#    set_target_properties(dsp PROPERTIES
#        WIN32_EXECUTABLE TRUE
#        LINK_FLAGS "/ENTRY:mainCRTStartup")
#endif()