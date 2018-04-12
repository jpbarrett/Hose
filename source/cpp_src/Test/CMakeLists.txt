option (HOSE_ENABLE_TEST "Build developer tests." OFF)
if (HOSE_ENABLE_TEST)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Devices/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Operators/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    set (HOSE_TEST_LIBS HCore HDevices)

    if(HOSE_USE_CUDA)

        include(FindCUDA)
        find_package(CUDA REQUIRED)
        include_directories(${CUDA_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS HCUDAOperators ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})

    endif(HOSE_USE_CUDA)

    set(SOURCE_BASENAMES
        TestBufferAllocator
        # TestDummyDigitizer
        TestMultiThreadDummy
    )

    if(HOSE_USE_CUDA)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CUDASpectrometer/include)
        list(APPEND HOSE_TEST_LIBS HCUDASpectrometer HCUDAOperators ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestMultiThreadDummySpectrometer)
    endif(HOSE_USE_CUDA)

    if(HOSE_USE_DIGITAL_RF)
        find_package(DigitalRF REQUIRED)
        include_directories(${DIGITALRF_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${DIGITALRF_LIBRARIES})
        link_directories(${DIGITALRF_LIBRARY_DIRS})
        list(APPEND SOURCE_BASENAMES TestMultiThreadDummyDRF)
        # set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DIGITALRF_CFLAGS}" )
        # list( APPEND CMAKE_EXE_LINKER_FLAGS ${DIGITALRF_LDLAGS} )
        message(STATUS "digital_rf libraries = ${DIGITALRF_LIBRARIES}")
        message(STATUS "digital_rf include dirs = ${DIGITALRF_INCLUDE_DIRS}")
        message(STATUS "digital_rf cflags = ${DIGITALRF_CFLAGS}")
        message(STATUS "digital_rf ldflags = ${DIGITALRF_LDFLAGS}")
    endif()

    if (HOSE_USE_ADQ7)
        find_package(ADQSDK REQUIRED)
        include_directories(${ADQSDK_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${ADQSDK_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestADQ7DigitizerInterface TestMultiThreadADQ7)
        if(HOSE_USE_CUDA)
        list(APPEND SOURCE_BASENAMES TestMultiThreadADQ7Spectrometer)
        endif(HOSE_USE_CUDA)
    endif()

    if (HOSE_USE_ROOT)
        include(FindROOT)
        find_package(ROOT REQUIRED)
        include_directories(${ROOT_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${ROOT_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestSpectrumPlot TestSpectrumDifference)
    endif()

    if (HOSE_USE_PX14)
        find_package(PX14400 REQUIRED)
        include_directories(${PX14400_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${PX14400_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestPX14DigitizerInterface TestMultiThreadPX14)

        if(HOSE_USE_DIGITAL_RF)
            list(APPEND SOURCE_BASENAMES TestMultiThreadPX14DRF)
        endif()

        if(HOSE_USE_CUDA)
            list(APPEND SOURCE_BASENAMES TestMultiThreadPX14Spectrometer)
        endif(HOSE_USE_CUDA)
    endif()

    foreach( BASENAME ${SOURCE_BASENAMES} )
        add_executable( ${BASENAME} ${BASENAME}.cc )
        target_link_libraries( ${BASENAME} ${HOSE_TEST_LIBS})
    endforeach( BASENAME )

    hose_install_executables( ${SOURCE_BASENAMES} )

endif (HOSE_ENABLE_TEST)
