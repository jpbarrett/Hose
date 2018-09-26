#cxx flags (have to set c++11 flag at sub-directory level to accomodate CUDA)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

option (HOSE_USE_FFTW "Use FFTW" OFF)
if (HOSE_USE_FFTW)
  find_package(FFTW REQUIRED)
  hose_external_include_directories (${FFTW_INCLUDE_DIRS})
  hose_add_cxxflag(HOSE_USE_FFTW)
endif (HOSE_USE_FFTW)

#include directories ###########################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#headers #######################################################################
set (HSIGNAL_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HBitReversalPermutation.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HFastFourierTransform.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HFastFourierTransformUtilities.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HMultidimensionalFastFourierTransform.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSimulatedAnalogSignalSampleGenerator.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HPowerLawNoiseSignal.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HGaussianWhiteNoiseSignal.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSwitchedSignal.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSummedSignal.hh
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HDummyUniformRawArrayFiller.hh
    # ${CMAKE_CURRENT_SOURCE_DIR}/include/HDummyGaussianRawArrayFiller.hh
)

#source ########################################################################
set (HSIGNAL_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HBitReversalPermutation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HFastFourierTransform.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HFastFourierTransformUtilities.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HPowerLawNoiseSignal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HGaussianWhiteNoiseSignal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HSwitchedSignal.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HSummedSignal.cc
)

#additional FFTW dependent files ###############################################
if (HOSE_USE_FFTW)
list(APPEND HSIGNAL_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HMultidimensionalFastFourierTransformFFTW.hh
)
endif (HOSE_USE_FFTW)

#compile and install library ###################################################

set(HSIGNAL_LIBS HInterface HCore)

if(HOSE_USE_FFTW)
    list(APPEND HSIGNAL_LIBS ${FFTW_LIBRARIES})
endif(HOSE_USE_FFTW)

add_library (HSignal SHARED ${HSIGNAL_SOURCEFILES})
target_link_libraries (HSignal ${HSIGNAL_LIBS})

hose_install_headers (${HSIGNAL_HEADERFILES})
hose_install_libraries (HSignal)


