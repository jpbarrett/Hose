include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Signal/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(HDEVICES_LIBS HCore HInterface HSignal)

set( HDEVICES_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDummyDigitizer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HPX14DigitizerSimulator.hh
)

set( HDEVICES_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HPX14DigitizerSimulator.cc
)

if (HOSE_USE_PX14)
    find_package(PX14400 REQUIRED)
    include_directories(${PX14400_INCLUDE_DIRS})
    list(APPEND HDEVICES_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HPX14BufferAllocator.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HPX14Digitizer.hh

    )
    list(APPEND HDEVICES_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HPX14BufferAllocator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HPX14Digitizer.cc
    )
    list(APPEND HDEVICES_LIBS ${PX14400_LIBRARIES})
endif()

if (HOSE_USE_ADQ7)
    find_package(ADQSDK REQUIRED)
    include_directories(${ADQSDK_INCLUDE_DIRS})
    list(APPEND HDEVICES_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HADQ7Digitizer.hh
    )
    list(APPEND HDEVICES_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HADQ7Digitizer.cc
    )
    list(APPEND HDEVICES_LIBS ${ADQSDK_LIBRARIES})
endif()

if (HOSE_USE_ATS)
    find_package(ATSSDK REQUIRED)
    include_directories(${ATSSDK_INCLUDE_DIRS})
    # list(APPEND HDEVICES_HEADERFILES
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include/HADQ7Digitizer.hh
    # )
    # list(APPEND HDEVICES_SOURCEFILES
    #     ${CMAKE_CURRENT_SOURCE_DIR}/src/HADQ7Digitizer.cc
    # )
    list(APPEND HDEVICES_LIBS ${ATSSDK_LIBRARIES})
endif()

##################################################

add_library (HDevices SHARED ${HDEVICES_SOURCEFILES})
target_link_libraries (HDevices ${HDEVICES_LIBS})

hose_install_headers (${HDEVICES_HEADERFILES})
hose_install_libraries (HDevices)
