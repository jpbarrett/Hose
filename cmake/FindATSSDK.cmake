#Try to find the ATS SDK library and headers

find_library(ATSSDK_LIBRARY	NAMES ATSApi)

find_path(ATSSDK_INCLUDE_DIR NAMES AlazarApi.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ATSSDK DEFAULT_MSG ATSSDK_LIBRARY	ATSSDK_INCLUDE_DIR)

if(ATSSDK_FOUND)
	set(ATSSDK_LIBRARIES "${ATSSDK_LIBRARY}")
	set(ATSSDK_INCLUDE_DIRS "${ATSSDK_INCLUDE_DIR}")
endif(ATSSDK_FOUND)

mark_as_advanced(ATSSDK_INCLUDE_DIRS ATSSDK_LIBRARIES)
