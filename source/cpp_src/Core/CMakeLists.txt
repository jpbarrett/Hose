#cxx flags (have to set c++11 flag at sub-directory level to accomodate CUDA)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#list header files #############################################################

set (HCORE_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HLinearBuffer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HChunkedRingBuffer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorBase.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorNew.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorMalloc.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDigitizer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferMetaData.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumObject.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HTimer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HThreadPool.hh
)

#list source files #############################################################

set( HCORE_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HTimer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HThreadPool.cc
)

#declare header paths ##########################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#compile and install library ###################################################

add_library (HCore SHARED ${HCORE_SOURCEFILES})
target_link_libraries (HCore ${HCORE_LIBS})

hose_install_headers (${HCORE_HEADERFILES})
hose_install_libraries (HCore)
