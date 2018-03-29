cmake_minimum_required(VERSION 3.0)
project(Hose)

if(POLICY CMP0053)
    cmake_policy(SET CMP0053 OLD)
endif()

# Module version
set(MODULE_VERSION_MAJOR 0)
set(MODULE_VERSION_MINOR 0)
set(MODULE_VERSION_PATCH 0)
set(MODULE_VERSION "${MODULE_VERSION_MAJOR}.${MODULE_VERSION_MINOR}.${MODULE_VERSION_PATCH}")
set(MODULE_VERSION_NUMBER "${MODULE_VERSION_MAJOR}.${MODULE_VERSION_MINOR}${MODULE_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )
include(HoseDefaults)
include(CMakeDependentOption)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "default install path" FORCE )
endif()

#define install directories
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/config")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/data")


file(MAKE_DIRECTORY ${INCLUDE_INSTALL_DIR})
file(MAKE_DIRECTORY ${LIB_INSTALL_DIR})
file(MAKE_DIRECTORY ${BIN_INSTALL_DIR})
file(MAKE_DIRECTORY ${CONFIG_INSTALL_DIR})
file(MAKE_DIRECTORY ${DATA_INSTALL_DIR})

hose_module_paths(Hose)

#compiler flags
hose_require_cpp11()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread" )
message(STATUS "Using compiler ${COMPILER_ID} ${COMPILER_VERSION}")

#options for triggering additional dependencies
option( HOSE_USE_PX14 "Build PX14400 device support" OFF )
option( HOSE_USE_ADQ7 "Build ADQ7 device support" OFF )
option( HOSE_USE_DIGITAL_RF "Build digital_rf I/O support" OFF )
option( HOSE_USE_CUDA "Build CUDA support." OFF)
option( HOSE_USE_ROOT "Build ROOT based utilities." OFF)

#convenience variables
set(CPPSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/cpp_src)
set(SHSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/bash_src)

#source sub directories (in order of dependence)
add_subdirectory(${CPPSOURCE}/Array)
add_subdirectory(${CPPSOURCE}/Core)
if(HOSE_USE_CUDA)
    add_subdirectory( ${CPPSOURCE}/CUDASpectrometer)
endif(HOSE_USE_CUDA)
add_subdirectory(${CPPSOURCE}/Operators)
add_subdirectory(${CPPSOURCE}/Devices)
add_subdirectory(${CPPSOURCE}/Test)

#env script
configure_file( ${SHSOURCE}/hoseenv.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/hoseenv.sh @ONLY)
install( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hoseenv.sh DESTINATION ${BIN_INSTALL_DIR} )
