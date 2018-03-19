#Try to find the ADQ SDK library and headers

find_library(ADQSDK_LIBRARY	NAMES adq)

find_path(ADQSDK_INCLUDE_DIR NAMES ADQAPI.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ADQSDK DEFAULT_MSG ADQSDK_LIBRARY	ADQSDK_INCLUDE_DIR)

if(ADQSDK_FOUND)
	set(ADQSDK_LIBRARIES "${ADQSDK_LIBRARY}")
	set(ADQSDK_INCLUDE_DIRS "${ADQSDK_INCLUDE_DIR}")
endif(ADQSDK_FOUND)

mark_as_advanced(ADQSDK_INCLUDE_DIRS ADQSDK_LIBRARIES)
