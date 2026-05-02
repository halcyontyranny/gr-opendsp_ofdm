cmake_minimum_required(VERSION 3.13)
project(gr-opendsp_ofdm CXX)
set(CMAKE_CXX_STANDARD 14)

# ── GNU Radio ─────────────────────────────────────────────────────────────────

list(INSERT CMAKE_MODULE_PATH 0 "/usr/lib/x86_64-linux-gnu/cmake/gnuradio")
find_package(Gnuradio "3.10" REQUIRED COMPONENTS runtime pmt)
include(GrPython)
include(GrPybind)

option(ENABLE_PYTHON "Build Python/pybind11 bindings (needed for GRC)" ON)

# ── opendsp-ofdm library ──────────────────────────────────────────────────────

set(OPENDSP_OFDM_DIR "${CMAKE_SOURCE_DIR}/../opendsp-ofdm"
    CACHE PATH "Path to opendsp-ofdm source tree")
set(OPENDSP_OFDM_BUILD_DIR "${OPENDSP_OFDM_DIR}/build"
    CACHE PATH "Path to opendsp-ofdm build directory (must contain libopendsp_ofdm_lib.a)")

if(NOT EXISTS "${OPENDSP_OFDM_BUILD_DIR}/libopendsp_ofdm_lib.a")
    message(FATAL_ERROR
        "libopendsp_ofdm_lib.a not found at ${OPENDSP_OFDM_BUILD_DIR}\n"
        "Build opendsp-ofdm first:\n"
        "  cd ${OPENDSP_OFDM_DIR} && mkdir build && cd build\n"
        "  cmake .. -DWITH_CODEC2=ON && make")
endif()

add_library(opendsp_ofdm_dep STATIC IMPORTED)
set_target_properties(opendsp_ofdm_dep PROPERTIES
    IMPORTED_LOCATION "${OPENDSP_OFDM_BUILD_DIR}/libopendsp_ofdm_lib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENDSP_OFDM_DIR}/include"
    INTERFACE_COMPILE_DEFINITIONS "HAVE_CODEC2")

# ── External deps shared with opendsp-ofdm ───────────────────────────────────

find_library(CODEC2_LIB NAMES codec2 REQUIRED)
find_library(FFTW3_LIB  NAMES fftw3  REQUIRED)

# ── Subdirectories ────────────────────────────────────────────────────────────

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(grc)
if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()
