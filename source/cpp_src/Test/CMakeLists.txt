if (HOSE_ENABLE_TEST)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Operators/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Devices/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Signal/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../MetaData/include)

    set (HOSE_TEST_LIBS HCore HInterface HSignal HOperators HMeta)

    set(SOURCE_BASENAMES
        TestBufferAllocator
        TestTimeStampConverter
        TestParameters
        # TestDummyDigitizer
        # TestMultiThreadDummy
        # TestMultiThreadDummySpectrometer
    )

    list(APPEND HOSE_TEST_LIBS HDevices)

    if(HOSE_USE_ZEROMQ)
        include(FindZeroMQ)
        find_package(ZeroMQ REQUIRED)
        include_directories(${ZMQ_INCLUDE_DIRS})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Server/include)
        list(APPEND HOSE_TEST_LIBS HServer ${ZMQ_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestServer)
    endif(HOSE_USE_ZEROMQ)

    if(HOSE_USE_SPDLOG)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Logging/include)
    endif(HOSE_USE_SPDLOG)

    if(HOSE_USE_CUDA)
        include(FindCUDA)
        find_package(CUDA REQUIRED)
        include_directories(${CUDA_INCLUDE_DIRS})
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CUDASpectrometer/include)
        list(APPEND HOSE_TEST_LIBS HCUDASpectrometer HCUDAOperators ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})
    endif(HOSE_USE_CUDA)

    if (HOSE_USE_ADQ7)
        find_package(ADQSDK REQUIRED)
        include_directories(${ADQSDK_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${ADQSDK_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestADQ7DigitizerInterface ) # TestMultiThreadADQ7)
        if(HOSE_USE_CUDA)
        list(APPEND SOURCE_BASENAMES TestMultiThreadADQ7Spectrometer)
        endif(HOSE_USE_CUDA)
    endif()

    if (HOSE_USE_ROOT)
        include(FindROOT)
        find_package(ROOT REQUIRED)
        include_directories(${ROOT_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${ROOT_LIBRARIES})
        list(APPEND SOURCE_BASENAMES  TestNoiseGenerator TestSummedNoiseGenerator)
    endif()

    if (HOSE_USE_PX14)
        find_package(PX14400 REQUIRED)
        include_directories(${PX14400_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${PX14400_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestPX14DigitizerInterface)

        if(HOSE_USE_DIGITAL_RF)
            list(APPEND SOURCE_BASENAMES TestMultiThreadPX14DRF)
        endif()

        # if(HOSE_USE_CUDA)
        #     list(APPEND SOURCE_BASENAMES TestMultiThreadPX14Spectrometer)
        # endif(HOSE_USE_CUDA)
    endif()

    if (HOSE_USE_ATS)
        find_package(ATSSDK REQUIRED)
        include_directories(${ATSSDK_INCLUDE_DIRS})
        list(APPEND HOSE_TEST_LIBS ${ATSSDK_LIBRARIES})
        list(APPEND SOURCE_BASENAMES TestATS9371DigitizerInterface TestMultiThreadATS9371)
    endif()

    foreach( BASENAME ${SOURCE_BASENAMES} )
        add_executable( ${BASENAME} ${BASENAME}.cc )
        target_link_libraries( ${BASENAME} ${HOSE_TEST_LIBS})
    endforeach( BASENAME )

    hose_install_executables( ${SOURCE_BASENAMES} )

endif (HOSE_ENABLE_TEST)
