if(HOSE_USE_CUDA)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    include(FindCUDA)
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})

    set(CUDASpectrometer_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/spectrometer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/noise_statistics_mbp_reduce.h
        # ${CMAKE_CURRENT_SOURCE_DIR}/include/spectrometer_signed.h
    )

    set(CUDASpectrometer_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spectrometer.cu
        ${CMAKE_CURRENT_SOURCE_DIR}/src/noise_statistics_mbp_reduce.cu
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/spectrometer_signed.cu
    )

    #diable C++11 for this directory, because only the newest 7+ CUDA compilers support it
    #set( CMAKE_C_FLAGS "-Wall -Wextra" )
    #set( CMAKE_CXX_FLAGS "-Wall -Wextra -pthread" )
    #set( CUDA_NVCC_FLAGS "--ptxas-options=-v -D_FORCE_INLINES --compiler-options '-fPIC' -shared")

    if(HOSE_USE_ADQ7)
       set( CMAKE_C_FLAGS "-Wall -Wextra -DHOSE_USE_ADQ7" )
       hose_add_cxxflag(HOSE_USE_ADQ7)
       hose_add_cflag(HOSE_USE_ADQ7)
    else()
    set( CMAKE_C_FLAGS "-Wall -Wextra" )
    set( CMAKE_CXX_FLAGS "-Wall -Wextra -pthread" )
    set( CUDA_NVCC_FLAGS "--ptxas-options=-v -D_FORCE_INLINES --compiler-options '-fPIC' -shared")
    endif(HOSE_USE_ADQ7)





    cuda_add_library( HCUDASpectrometer SHARED ${CUDASpectrometer_SOURCEFILES} )
    target_link_libraries ( HCUDASpectrometer ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES} )

    hose_install_headers( ${CUDASpectrometer_HEADERFILES} )
    hose_install_libraries( HCUDASpectrometer )

endif(HOSE_USE_CUDA)
