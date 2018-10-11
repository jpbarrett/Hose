include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Signal/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Devices/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Operators/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CUDASpectrometer/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../MetaData/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(HOSE_USE_SPDLOG)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Logging/include)
endif(HOSE_USE_SPDLOG)

set(HOSE_MAIN_LIBS HCore HInterface HDevices HOperators)

if(HOSE_USE_CUDA AND HOSE_USE_ZEROMQ)
    set(SOURCE_BASENAMES
        RunSpectrometer
        LaunchSpectrometerDaemon
    )
endif(HOSE_USE_CUDA AND HOSE_USE_ZEROMQ)

if( HOSE_USE_ZEROMQ)
    include(FindZeroMQ)
    find_package(ZeroMQ REQUIRED)
    include_directories(${ZMQ_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Server/include)
    list(APPEND HOSE_MAIN_LIBS HServer ${ZMQ_LIBRARIES})
endif(HOSE_USE_ZEROMQ)

if(HOSE_USE_CUDA)
    include(FindCUDA)
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})
    list(APPEND HOSE_MAIN_LIBS HCUDASpectrometer HCUDAOperators ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})
endif(HOSE_USE_CUDA)

if(HOSE_USE_PX14)
    find_package(PX14400 REQUIRED)
    include_directories(${PX14400_INCLUDE_DIRS})
    list(APPEND HOSE_MAIN_LIBS ${PX14400_LIBRARIES})
    hose_add_cxxflag(HOSE_USE_PX14)
endif(HOSE_USE_PX14)

if (HOSE_USE_ROOT)
    include(FindROOT)
    find_package(ROOT REQUIRED)
    include_directories(${ROOT_INCLUDE_DIRS})
    list(APPEND HOSE_MAIN_LIBS ${ROOT_LIBRARIES})
    list(APPEND SOURCE_BASENAMES SpectrumPlot RawPlot PowerPlot)
endif()

foreach( BASENAME ${SOURCE_BASENAMES} )
    add_executable( ${BASENAME} ${BASENAME}.cc )
    target_link_libraries( ${BASENAME} ${HOSE_MAIN_LIBS})
endforeach( BASENAME )

hose_install_executables( ${SOURCE_BASENAMES} )
