#list header files #############################################################

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set (HARRAY_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HLinearBuffer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HChunkedRingBuffer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorBase.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorNew.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorMalloc.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDigitizer.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferMetaData.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumObject.hh
)

#declare header paths ##########################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#install library (headers only) ################################################

hose_install_headers (${HARRAY_HEADERFILES})
