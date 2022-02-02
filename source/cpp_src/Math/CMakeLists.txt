#cxx flags (have to set c++11 flag at sub-directory level to accomodate CUDA)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#include directories ###########################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#headers #######################################################################
set (HMATH_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BoxConvolvedWithGaussian.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BoxConvolvedWithGaussianIntegrand.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BoxConvolvedWithGaussianPDF.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BoxPDF.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GaussianPDF.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/GSLIntegrationInterface.hh
)

#source ########################################################################
set (HMATH_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BoxConvolvedWithGaussianIntegrand.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BoxConvolvedWithGaussianPDF.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BoxPDF.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GaussianPDF.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GSLIntegrationInterface.cc
)


#compile and install library ###################################################
set(HMATH_LIBS HCore) 

include(FindROOT)
find_package(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIRS})
list(APPEND HMATH_LIBS ${ROOT_LIBRARIES})

include(FindGSL)
find_package(GSL REQUIRED)
include_directories(${GSL_INCLUDE_DIRS})
list(APPEND HMATH_LIBS ${GSL_LIBRARIES})

add_library (HMath SHARED ${HMATH_SOURCEFILES})
target_link_libraries (HMath ${HMATH_LIBS})

hose_install_headers (${HMATH_HEADERFILES})
hose_install_libraries (HMath)


