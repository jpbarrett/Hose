if (PY_EXE)

    #configure and install scripts
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/hoseclient.py.in" "${CMAKE_CURRENT_BINARY_DIR}/hoseclient.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/hoseclient.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/hosecc.py.in" "${CMAKE_CURRENT_BINARY_DIR}/hosecc.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/hosecc.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/create-schedule.py.in" "${CMAKE_CURRENT_BINARY_DIR}/create-schedule.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/create-schedule.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/replay-log.py.in" "${CMAKE_CURRENT_BINARY_DIR}/replay-log.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/replay-log.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/capture-fs-log.py.in" "${CMAKE_CURRENT_BINARY_DIR}/capture-fs-log.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/capture-fs-log.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/capture-spectrometer-log.py.in" "${CMAKE_CURRENT_BINARY_DIR}/capture-spectrometer-log.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/capture-spectrometer-log.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/capture-encoder-log.py.in" "${CMAKE_CURRENT_BINARY_DIR}/capture-encoder-log.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/capture-encoder-log.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/create-meta-data.py.in" "${CMAKE_CURRENT_BINARY_DIR}/create-meta-data.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/create-meta-data.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/monitor-spectrometer.py.in" "${CMAKE_CURRENT_BINARY_DIR}/monitor-spectrometer.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/monitor-spectrometer.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_freq_conversion.py.in" "${CMAKE_CURRENT_BINARY_DIR}/test_freq_conversion.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/test_freq_conversion.py" DESTINATION ${BIN_INSTALL_DIR})

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/check_luff.py.in" "${CMAKE_CURRENT_BINARY_DIR}/check_luff.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/check_luff.py" DESTINATION ${BIN_INSTALL_DIR})

endif (PY_EXE)
