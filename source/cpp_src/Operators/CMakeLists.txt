
#list header files #############################################################

set (HOPERATORS_HEADERFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSimpleMultiThreadedWriter.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HRawDataDumper.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HSwitchedPowerCalculator.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HDataAccumulationWriter.hh
    ${CMAKE_CURRENT_SOURCE_DIR}/include/HAveragedMultiThreadedSpectrumDataWriter.hh
)

set (HOPERATORS_SOURCEFILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HDataAccumulationWriter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HAveragedMultiThreadedSpectrumDataWriter.cc
)

#declare header paths ##########################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#install the library ###############################################

add_library (HOperators SHARED ${HOPERATORS_SOURCEFILES})
target_link_libraries ( HOperators HInterface HCore )

hose_install_headers( ${HOPERATORS_HEADERFILES} )
hose_install_libraries( HOperators )

#if cuda is enabled, compile and install the cuda files

if(HOSE_USE_CUDA)

    include(FindCUDA)
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CUDASpectrometer/include)

    set(CUDAOPERATOR_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrometerCUDA.hh
        # ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrometerCUDASigned.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HCudaHostBufferAllocator.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HBufferAllocatorSpectrometerDataCUDA.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HSimpleMultiThreadedSpectrumDataWriter.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HSpectrumAverager.hh
    )

    set(CUDAOPERATOR_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HSpectrometerCUDA.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HSpectrumAverager.cc
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/HSpectrometerCUDASigned.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HSimpleMultiThreadedSpectrumDataWriter.cc
    )

    add_library (HCUDAOperators SHARED ${CUDAOPERATOR_SOURCEFILES})
    target_link_libraries ( HCUDAOperators HInterface HCore HCUDASpectrometer )

    hose_install_headers( ${CUDAOPERATOR_HEADERFILES} )
    hose_install_libraries( HCUDAOperators )

endif(HOSE_USE_CUDA)
