cmake_minimum_required(VERSION 3.0)
project(Hose)

# if(POLICY CMP0053)
#     cmake_policy(SET CMP0053 OLD)
# endif()

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
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/data" CACHE PATH "path to the directory were data will be recorded")

set(LOG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/log")


file(MAKE_DIRECTORY ${INCLUDE_INSTALL_DIR})
file(MAKE_DIRECTORY ${LIB_INSTALL_DIR})
file(MAKE_DIRECTORY ${BIN_INSTALL_DIR})
file(MAKE_DIRECTORY ${CONFIG_INSTALL_DIR})
file(MAKE_DIRECTORY ${DATA_INSTALL_DIR})
file(MAKE_DIRECTORY ${LOG_INSTALL_DIR})

hose_module_paths(Hose)

#compiler flags
hose_require_cpp11()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread" )
message(STATUS "Using compiler ${COMPILER_ID} ${COMPILER_VERSION}")

#options for triggering additional dependencies
option( HOSE_USE_PX14 "Build PX14400 device support" OFF )
option( HOSE_USE_ADQ7 "Build ADQ7 device support" OFF )
option( HOSE_USE_ATS "Build AlazarTech device support" OFF )
option( HOSE_USE_CUDA "Build CUDA support." OFF)
option( HOSE_USE_ROOT "Build ROOT based utilities." OFF)
option( HOSE_USE_ZEROMQ "Build client-server library" OFF )
option( HOSE_USE_SPDLOG "Build logging support using spdlog library" ON)
option( HOSE_ENABLE_TEST "Build developer tests." OFF)
option( HOSE_ADQ7_SOFTWARE_TRIGGER "Enable software trigger for ADQ7" OFF)
option( HOSE_USE_GSL "Build GSL based utilities" OFF)

if(HOSE_USE_ZEROMQ)
    hose_add_cxxflag(HOSE_USE_ZEROMQ)
endif(HOSE_USE_ZEROMQ)

if(HOSE_USE_SPDLOG)
    hose_add_cxxflag(HOSE_USE_SPDLOG)
endif(HOSE_USE_SPDLOG)

if(HOSE_USE_ADQ7)
    hose_add_cxxflag(HOSE_USE_ADQ7)
    hose_add_cflag(HOSE_USE_ADQ7)
endif(HOSE_USE_ADQ7)

#convenience variables
set(CSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/c_src)
set(CPPSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/cpp_src)
set(SHSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/bash_src)
set(PYSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/python_src)

#source sub directories (in order of dependence)
add_subdirectory(${CSOURCE}/Interface)
add_subdirectory(${CPPSOURCE}/Array)
add_subdirectory(${CPPSOURCE}/Core)
add_subdirectory(${CPPSOURCE}/MetaData)
add_subdirectory(${CPPSOURCE}/Signal)
if(HOSE_USE_ROOT AND HOSE_USE_GSL)
    add_subdirectory( ${CPPSOURCE}/Math)
endif()

add_subdirectory(${CPPSOURCE}/Server)
if(HOSE_USE_CUDA)
    add_subdirectory( ${CPPSOURCE}/CUDASpectrometer)
endif(HOSE_USE_CUDA)
add_subdirectory(${CPPSOURCE}/Operators)
add_subdirectory(${CPPSOURCE}/Devices)
add_subdirectory(${CPPSOURCE}/Main)
add_subdirectory(${CPPSOURCE}/Test)


if(${CMAKE_VERSION} VERSION_GREATER "3.12.0")


  find_package (Python3 COMPONENTS Interpreter Development)

  if (Python3_EXECUTABLE)
      #figure out the python install path
      #so we can add it to PYTHONPATH
      file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pypath.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      )
      execute_process(COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/pypath.py"  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_VARIABLE PYTHON_SITE_PREFIX)
      message(STATUS "Python path = ${PYTHON_SITE_PREFIX}")

      string(STRIP "${PYTHON_SITE_PREFIX}" PYTHON_SITE_PREFIX)

      set( PYTHON_MODULE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE_PREFIX}")

      #now add the python source dir
      set(PY_EXE "${Python3_EXECUTABLE}")
      add_subdirectory(${PYSOURCE})
  endif (Python3_EXECUTABLE)

else()

  #if CMAKE version < 3.12 use old style find package to figure out where python is
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)

  if (PYTHON_EXECUTABLE)
      #figure out the python install path
      #so we can add it to PYTHONPATH
      file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pypath.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      )
      execute_process(COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/pypath.py"  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} OUTPUT_VARIABLE PYTHON_SITE_PREFIX)
      message(STATUS "Python path = ${PYTHON_SITE_PREFIX}")

      string(STRIP "${PYTHON_SITE_PREFIX}" PYTHON_SITE_PREFIX)

      set( PYTHON_MODULE_INSTALL_DIR  "${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE_PREFIX}")

      #now add the python source dir
      set(PY_EXE "${PYTHON_EXECUTABLE}")
      add_subdirectory(${PYSOURCE})
  endif (PYTHON_EXECUTABLE)

endif()


#configure env script
configure_file( ${SHSOURCE}/hoseenv.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/hoseenv.sh @ONLY)
install( PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hoseenv.sh DESTINATION ${BIN_INSTALL_DIR} )
