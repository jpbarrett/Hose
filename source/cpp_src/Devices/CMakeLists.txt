include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Devices/include)


set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(DEVICE_LIBS "")

set( DEVICE_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDummyDigitizer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDummyDigitizerSigned.hh
)

set( DEVICE_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HDummyDigitizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HDummyDigitizerSigned.cc
)

if (HOSE_USE_PX14)
    find_package(PX14400 REQUIRED)
    include_directories(${PX14400_INCLUDE_DIRS})
    list(APPEND DEVICE_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HPX14BufferAllocator.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HPX14Digitizer.hh
    )
    list(APPEND DEVICE_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HPX14BufferAllocator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HPX14Digitizer.cc
    )
    list(APPEND DEVICE_LIBS ${PX14400_LIBRARIES})
endif()

if (HOSE_USE_ADQ7)
    find_package(ADQSDK REQUIRED)
    include_directories(${ADQSDK_INCLUDE_DIRS})
    list(APPEND DEVICE_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HADQ7Digitizer.hh
    )
    list(APPEND DEVICE_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HADQ7Digitizer.cc
    )
    list(APPEND DEVICE_LIBS ${ADQSDK_LIBRARIES})
endif()

##################################################

add_library (HDevices SHARED ${DEVICE_SOURCEFILES})
target_link_libraries (HDevices ${DEVICE_LIBS})

hose_install_headers (${DEVICE_HEADERFILES})
hose_install_libraries (HDevices)
