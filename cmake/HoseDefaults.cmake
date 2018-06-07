macro (hose_add_cflag CFLAG)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${CFLAG}")
endmacro()

macro (hose_add_cxxflag CFLAG)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${CFLAG}")
endmacro()

macro( hose_module_paths PATH )

    set( ${PROJECT_NAME}_INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_DIR} )
    set( ${PROJECT_NAME}_LIB_INSTALL_DIR ${LIB_INSTALL_DIR} )
    set( ${PROJECT_NAME}_BIN_INSTALL_DIR ${BIN_INSTALL_DIR} )
    set( ${PROJECT_NAME}_CONFIG_INSTALL_DIR ${CONFIG_INSTALL_DIR}/${PATH} )
    set( ${PROJECT_NAME}_DATA_INSTALL_DIR ${DATA_INSTALL_DIR}/${PATH} )
    set( ${PROJECT_NAME}_LOG_INSTALL_DIR ${LOG_INSTALL_DIR}/${PATH} )
    # set( ${PROJECT_NAME}_SHARE_INSTALL_DIR ${SHARE_INSTALL_DIR}/${PATH} )

    #don't need these defines at the moment
    #add_definitions( -D${PROJECT_NAME}_INSTALL_DIR=${CMAKE_INSTALL_PREFIX} )
    #add_definitions( -D${PROJECT_NAME}_INCLUDE_INSTALL_DIR=${${PROJECT_NAME}_INCLUDE_INSTALL_DIR} )
    install(CODE "file(MAKE_DIRECTORY \"${${PROJECT_NAME}_INCLUDE_INSTALL_DIR}\")" )
    #add_definitions( -D${PROJECT_NAME}_LIB_INSTALL_DIR=${${PROJECT_NAME}_LIB_INSTALL_DIR} )
    install(CODE "file(MAKE_DIRECTORY \"${${PROJECT_NAME}_LIB_INSTALL_DIR}\")" )
    #add_definitions( -D${PROJECT_NAME}_BIN_INSTALL_DIR=${${PROJECT_NAME}_BIN_INSTALL_DIR} )
    install(CODE "file(MAKE_DIRECTORY \"${${PROJECT_NAME}_BIN_INSTALL_DIR}\")" )
    #add_definitions( -D${PROJECT_NAME}_CONFIG_INSTALL_DIR=${${PROJECT_NAME}_CONFIG_INSTALL_DIR} )
    install(CODE "file(MAKE_DIRECTORY \"${${PROJECT_NAME}_CONFIG_INSTALL_DIR}\")" )
    # add_definitions( -DDATA_INSTALL_DIR="${DATA_INSTALL_DIR}" )
    hose_add_cxxflag( DATA_INSTALL_DIR="${DATA_INSTALL_DIR}" )
    install(CODE "file(MAKE_DIRECTORY \"${DATA_INSTALL_DIR}\")" )
    # add_definitions( -DLOG_INSTALL_DIR="${LOG_INSTALL_DIR}" )
    hose_add_cxxflag( LOG_INSTALL_DIR="${LOG_INSTALL_DIR}" )
    install(CODE "file(MAKE_DIRECTORY \"${LOG_INSTALL_DIR}\")" )
    #add_definitions( -D${PROJECT_NAME}_SHARE_INSTALL_DIR=${${PROJECT_NAME}_DATA_INSTALL_DIR} )
    # install(CODE "file(MAKE_DIRECTORY \"${${PROJECT_NAME}_SHARE_INSTALL_DIR}\")" )
endmacro()


macro(hose_install_headers)
    install(FILES ${ARGN} DESTINATION ${INCLUDE_INSTALL_DIR})
endmacro()

macro(hose_install_libraries)
    install(TARGETS ${ARGN} EXPORT hoseTargets DESTINATION ${LIB_INSTALL_DIR})
    set_property(GLOBAL APPEND PROPERTY MODULE_TARGETS ${ARGN})
    set_target_properties(${ARGN} PROPERTIES INSTALL_NAME_DIR ${LIB_INSTALL_DIR})
endmacro()

macro(hose_install_executables)
    install(TARGETS ${ARGN} EXPORT hoseTargets DESTINATION ${BIN_INSTALL_DIR})
endmacro()


macro(hose_install_data)
    install(FILES ${ARGN} DESTINATION ${${PROJECT_NAME}_DATA_INSTALL_DIR})
endmacro()

macro(hose_install_config)
    install(FILES ${ARGN} DESTINATION ${${PROJECT_NAME}_CONFIG_INSTALL_DIR})
endmacro()

# macro(hose_install_share)
#     install(FILES ${ARGN} DESTINATION ${${PROJECT_NAME}_SHARE_INSTALL_DIR})
# endmacro()

#to match automake build
macro(hose_install_share_text)
    install(FILES ${ARGN} DESTINATION ${SHARE_TEXT_INSTALL_DIR})
endmacro()

# Compiler version check and C++11 support
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(COMPILER_IS_GNU 1)
    set(COMPILER_ID "GNU")
    if(NOT CMAKE_CXX_COMPILER_VERSION)
        exec_program(${CMAKE_C_COMPILER} ARGS "-dumpversion" OUTPUT_VARIABLE CMAKE_CXX_COMPILER_VERSION)
    endif()
    set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(COMPILER_IS_XCODE 1)
        set(COMPILER_ID "Xcode")
        if(NOT CMAKE_CXX_COMPILER_VERSION)
            exec_program(${CMAKE_C_COMPILER} ARGS "-v" OUTPUT_VARIABLE _xcode_version_info)
            string(REGEX REPLACE "^.*\\ ([0-9]+\\.[0-9]+).*$" "\\1" CMAKE_CXX_COMPILER_VERSION "${_xcode_version_info}")
        endif()
    else()
        set(COMPILER_IS_CLANG 1)
        set(COMPILER_ID "Clang")
        if(NOT CMAKE_CXX_COMPILER_VERSION)
            exec_program(${CMAKE_C_COMPILER} ARGS "-v" OUTPUT_VARIABLE _clang_version_info)
            string(REGEX REPLACE "^.*\\ ([0-9]+\\.[0-9]+).*$" "\\1" CMAKE_CXX_COMPILER_VERSION "${_clang_version_info}")
        endif()
    endif()
    set(COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    
else()
    message(FATAL_ERROR "Unknown or unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

macro(hose_require_clang_version VERSION)
    if(COMPILER_IS_CLANG)
        if(COMPILER_VERSION VERSION_LESS ${VERSION})
            message(FATAL_ERROR "Clang version >= ${VERSION} is required.")
        endif()
    endif()
endmacro()

macro(hose_require_xcode_version VERSION)
    if(COMPILER_IS_XCODE)
        if(COMPILER_VERSION VERSION_LESS ${VERSION})
            message(FATAL_ERROR "Xcode Clang version >= ${VERSION} is required.")
        endif()
    endif()
endmacro()

macro(hose_require_gcc_version VERSION)
    if(COMPILER_IS_GNU)
        if(COMPILER_VERSION VERSION_LESS ${VERSION})
            message(FATAL_ERROR "GCC version >= ${VERSION} is required.")
        endif()
    endif()
endmacro()

macro(hose_require_cpp11)
    if (COMPILER_IS_GNU AND NOT COMPILER_VERSION VERSION_LESS "6.0.0")
        # do nothing, g++ 6.x has c++14 support activated by default#
    else()
        if(COMPILER_SUPPORTS_CXX11)
            set( CXX11_FLAG "-std=c++11" )
        elseif(COMPILER_SUPPORTS_CXX0X)
            set( CXX11_FLAG "-std=c++0x" )
        else()
            message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
        endif()
        if (NOT "${CMAKE_CXX_FLAGS}" MATCHES "\\-std\\=c\\+\\+")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAG}" )
        endif()
        SET_PROPERTY(GLOBAL PROPERTY CXX11_FLAG ${CXX11_FLAG})
    endif()
endmacro()

macro( hose_include_directories )

    set(hose_DIRS)
    set(SYSTEM_DIRS)

    foreach(dir ${ARGN})
        string(FIND ${dir} ${CMAKE_SOURCE_DIR} pos)
        if(pos LESS 0)
            list(APPEND SYSTEM_DIRS ${dir})
        else()
            list(APPEND hose_DIRS ${dir})
        endif()
    endforeach()

    include_directories( ${hose_DIRS} )
    include_directories( SYSTEM;${SYSTEM_DIRS} )
endmacro()

macro( hose_internal_include_directories )

    foreach(dir ${ARGN})
        #message("${dir}")
        if(NOT IS_ABSOLUTE ${dir})
            set(dir ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        endif()
        list(APPEND MODULE_HEADER_DIRS ${dir})
    endforeach()

    list( LENGTH MODULE_HEADER_DIRS LL)
    if( LL GREATER 1 )
        list( REMOVE_DUPLICATES MODULE_HEADER_DIRS )
    endif()

    if (CMAKE_CURRENT_SOURCE_DIR STRGREATER PROJECT_SOURCE_DIR)
        set (MODULE_HEADER_DIRS ${MODULE_HEADER_DIRS} PARENT_SCOPE)
    endif()

    hose_include_directories( ${MODULE_HEADER_DIRS} )

endmacro()


macro( hose_external_include_directories )

    list( APPEND EXTERNAL_INCLUDE_DIRS ${ARGN} )

    list( LENGTH EXTERNAL_INCLUDE_DIRS LL)
    if( LL GREATER 1 )
        list( REMOVE_ITEM EXTERNAL_INCLUDE_DIRS SYSTEM )
        list( REMOVE_DUPLICATES EXTERNAL_INCLUDE_DIRS )
    endif()

    if (CMAKE_CURRENT_SOURCE_DIR STRGREATER PROJECT_SOURCE_DIR)
        #message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR} -- PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
        set( EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} PARENT_SCOPE )
    endif()

    hose_include_directories( ${ARGN} )

endmacro()


macro (hose_configure_automake RELPATH_LIST LIBLIST LIBNAME HEADER_LIST SOURCE_LIST)

    #construct the list of include paths
    list(APPEND AM_INCPATH_LIST "-I$(srcdir)/include")
    foreach(rpath ${RELPATH_LIST})
        list(APPEND AM_INCPATH_LIST "-I$(srcdir)/${rpath}/include")
    endforeach(rpath)

    #constuct the list of libs+relative paths needed by libtool
    list(LENGTH RELPATH_LIST tmplen)
    math(EXPR NLIBS "${tmplen} - 1")

    foreach(index RANGE ${NLIBS})
      list(GET LIBLIST ${index} lib)
      list(GET RELPATH_LIST ${index} librpath)
      list(APPEND AM_LIBLIST "${librpath}/lib${lib}.la")
    endforeach(index)

    string (REPLACE ";" " " HEADER_INCPATH_STR "${AM_INCPATH_LIST}")
    string (REPLACE ";" " " HEADER_BASENAMES_STR "${HEADER_LIST}")
    string (REPLACE ";" " " SOURCE_BASENAMES_STR "${SOURCE_LIST}")
    string (REPLACE ";" " " AM_LIBLIST_STR "${AM_LIBLIST}")

    set(MAKEFILE_AM_IN "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am.in")
    set(MAKEFILE_AM    "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am")

    configure_file(${MAKEFILE_AM_IN} ${MAKEFILE_AM} @ONLY)

endmacro()

macro (hose_configure_exe_automake RELPATH_LIST INTERNAL_LIBLIST EXENAME HEADER_LIST SOURCE_LIST)

    #construct the list of include paths
    list(APPEND AM_INCPATH_LIST "-I$(srcdir)/include")
    foreach(rpath ${RELPATH_LIST})
        list(APPEND AM_INCPATH_LIST "-I$(srcdir)/${rpath}/include")
    endforeach(rpath)

    #constuct the list of libs+relative paths needed by libtool
    list(LENGTH RELPATH_LIST tmplen)
    math(EXPR NLIBS "${tmplen} - 1")

    foreach(index RANGE ${NLIBS})
      list(GET INTERNAL_LIBLIST ${index} lib)
      list(GET RELPATH_LIST ${index} librpath)
      list(APPEND AM_LIBLIST "${librpath}/lib${lib}.la")
    endforeach(index)

    string (REPLACE ";" " " HEADER_INCPATH_STR "${AM_INCPATH_LIST}")
    string (REPLACE ";" " " HEADER_BASENAMES_STR "${HEADER_LIST}")
    string (REPLACE ";" " " SOURCE_BASENAMES_STR "${SOURCE_LIST}")
    string (REPLACE ";" " " AM_LIBLIST_STR "${AM_LIBLIST}")

    set(MAKEFILE_AM_IN "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am.in")
    set(MAKEFILE_AM    "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.am")

    configure_file(${MAKEFILE_AM_IN} ${MAKEFILE_AM} @ONLY)

endmacro()
