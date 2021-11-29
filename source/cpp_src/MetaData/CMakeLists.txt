#cxx flags (have to set c++11 flag at sub-directory level to accomodate CUDA)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#include directories ###########################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#headers #######################################################################
set (HMETA_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HParameters.hh
)

#source ########################################################################
set (HMETA_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HParameters.cc
)

#compile and install library ###################################################
if(HOSE_USE_PX14)
    hose_add_cxxflag(HOSE_USE_PX14)
endif(HOSE_USE_PX14)

if(HOSE_USE_ADQ7)
    hose_add_cxxflag(HOSE_USE_ADQ7)
endif(HOSE_USE_ADQ7)



set(HMETA_LIBS HCore)

add_library (HMeta SHARED ${HMETA_SOURCEFILES})
target_link_libraries (HMeta ${HMETA_LIBS})

hose_install_headers (${HMETA_HEADERFILES})
hose_install_libraries (HMeta)
