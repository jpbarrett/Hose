if(HOSE_USE_CUDA AND HOSE_USE_ZEROMQ AND HOSE_USE_PX14)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Devices/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Operators/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CUDASpectrometer/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    set(HOSE_MAIN_LIBS HCore HDevices)

    include(FindZeroMQ)
    find_package(ZeroMQ REQUIRED)
    include_directories(${ZMQ_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Server/include)
    list(APPEND HOSE_MAIN_LIBS HServer ${ZMQ_LIBRARIES})

    include(FindCUDA)
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})
    list(APPEND HOSE_MAIN_LIBS HCUDASpectrometer HCUDAOperators ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})

    find_package(PX14400 REQUIRED)
    include_directories(${PX14400_INCLUDE_DIRS})
    list(APPEND HOSE_MAIN_LIBS ${PX14400_LIBRARIES})

    set(SOURCE_BASENAMES
        RunSpectrometer
    )

    foreach( BASENAME ${SOURCE_BASENAMES} )
        add_executable( ${BASENAME} ${BASENAME}.cc )
        target_link_libraries( ${BASENAME} ${HOSE_MAIN_LIBS})
    endforeach( BASENAME )

    hose_install_executables( ${SOURCE_BASENAMES} )

endif(HOSE_USE_CUDA AND HOSE_USE_ZEROMQ AND HOSE_USE_PX14)