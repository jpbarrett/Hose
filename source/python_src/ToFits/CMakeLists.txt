set(abs_srcdir "${CMAKE_CURRENT_SOURCE_DIR}")

if (PYTHON_EXECUTABLE)

    set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
    set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
    set(DEPS        "${CMAKE_CURRENT_SOURCE_DIR}/gpu_read/__init__.py")
    set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}/build/gpu_read_timestamp")

    configure_file(${SETUP_PY_IN} ${SETUP_PY})

    add_custom_command(OUTPUT ${OUTPUT}
                       COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} build
                       COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
                       DEPENDS ${DEPS})

    add_custom_target(gpu_read_target ALL DEPENDS ${OUTPUT})

    install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SETUP_PY} install --prefix=${CMAKE_INSTALL_PREFIX} )")

    #configure and install conversion script
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/gpu_sdfits.py.in" "${CMAKE_CURRENT_BINARY_DIR}/gpu_sdfits.py" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/gpu_sdfits.py" DESTINATION ${BIN_INSTALL_DIR})

endif (PYTHON_EXECUTABLE)
