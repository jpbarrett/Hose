include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HInterface_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBasicDefines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumHeaderStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumFileStruct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumFile.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HDataAccumulationStruct.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerHeaderStruct.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerFileStruct.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HNoisePowerFile.h
)

set(HInterface_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CreateSpectrumFileStruct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InitializeSpectrumFileStruct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ClearSpectrumFileStruct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DestroySpectrumFileStruct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WriteSpectrumFile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ReadSpectrumFile.c
)

#this is a c library
set( CMAKE_C_FLAGS "-Wall -Wextra" )

add_library( HInterface SHARED ${HInterface_SOURCEFILES} )
target_link_libraries ( HInterface )

hose_install_headers( ${HInterface_HEADERFILES} )
hose_install_libraries( HInterface )
