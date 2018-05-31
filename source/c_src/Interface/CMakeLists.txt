include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HInterface_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBasicSizes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDataAccumulationStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumHeaderStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumFileStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerHeaderStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerFileStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerFile.h
)

set(HInterface_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InitializeSpectrumObject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ClearSpectrumObject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ReadSpectrumObject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WriteSpectrumObject.c
)

#this is a c library
set( CMAKE_C_FLAGS "-Wall -Wextra" )

add_library( HInterface SHARED ${HInterface_SOURCEFILES} )
target_link_libraries ( HInterface )

hose_install_headers( ${HInterface_HEADERFILES} )
hose_install_libraries( HInterface )
