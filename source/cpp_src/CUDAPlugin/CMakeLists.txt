if(HOSE_USE_CUDA)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    include(FindCUDA)
    find_package(CUDA REQUIRED)
    include_directories(${CUDA_INCLUDE_DIRS})

    set(CUDAPLUGIN_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/spectrometer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/spectrometer_signed.h
    )

    set(CUDAPLUGIN_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spectrometer.cu
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spectrometer_signed.cu
    )

    #diable C++11 for this directory, because only the newest 7+ CUDA compilers support it
    set( CMAKE_C_FLAGS "-Wall -Wextra" )
    set( CMAKE_CXX_FLAGS "-Wall -Wextra -pthread" )
    set( CUDA_NVCC_FLAGS "--ptxas-options=-v -D_FORCE_INLINES --compiler-options '-fPIC' -shared")

    cuda_add_library( HCUDAPlugin SHARED ${CUDAPLUGIN_SOURCEFILES} )
    target_link_libraries ( HCUDAPlugin ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES} )

    hose_install_headers( ${CUDAPLUGIN_HEADERFILES} )
    hose_install_libraries( HCUDAPlugin )

endif(HOSE_USE_CUDA)
