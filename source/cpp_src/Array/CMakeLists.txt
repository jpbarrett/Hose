#list header files #############################################################

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set (HARRAY_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HArrayFillingOperator.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HArrayMath.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HArrayOperator.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HArrayWrapper.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBinaryArrayOperator.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HUnaryArrayOperator.hh
)

#declare header paths ##########################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#install library (headers only) ################################################

hose_install_headers (${HARRAY_HEADERFILES})
