if (PYTHON_EXECUTABLE)

    #configure and install scripts
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/hoseclient.py.in" "${CMAKE_CURRENT_BINARY_DIR}/hoseclient.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/hoseclient.py" DESTINATION ${BIN_INSTALL_DIR})

    #configure and install scripts
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/hosecc.py.in" "${CMAKE_CURRENT_BINARY_DIR}/hosecc.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/hosecc.py" DESTINATION ${BIN_INSTALL_DIR})

    #configure and install scripts
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/create-schedule.py.in" "${CMAKE_CURRENT_BINARY_DIR}/create-schedule.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/create-schedule.py" DESTINATION ${BIN_INSTALL_DIR})

endif (PYTHON_EXECUTABLE)
