include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(HOSE_USE_ZEROMQ)

    include(FindZeroMQ)
    find_package(ZeroMQ REQUIRED)
    include_directories(${ZMQ_INCLUDE_DIRS})

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../c_src/Interface/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Array/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Core/include)

    if(HOSE_USE_SPDLOG)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Logging/include)
    endif(HOSE_USE_SPDLOG)

    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    set(HSERVER_LIBS HCore)

    list(APPEND HSERVER_HEADERFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HApplicationBackend.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/HServer.hh
    )

    list(APPEND HSERVER_SOURCEFILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/HServer.cc
    )

    list(APPEND HSERVER_LIBS ${ZMQ_LIBRARIES})

    ##################################################

    add_library (HServer SHARED ${HSERVER_SOURCEFILES})
    target_link_libraries (HServer ${HSERVER_LIBS})

    hose_install_headers (${HSERVER_HEADERFILES})
    hose_install_libraries (HServer)

endif(HOSE_USE_ZEROMQ)
