#Try to find the PX14400 library and headers

find_library(PX14400_LIBRARY NAMES sig_px14400)

find_path(PX14400_INCLUDE_DIR NAMES px14.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PX14400 DEFAULT_MSG PX14400_LIBRARY	PX14400_INCLUDE_DIR)

if(PX14400_FOUND)
	set(PX14400_LIBRARIES "${PX14400_LIBRARY}")
	set(PX14400_INCLUDE_DIRS "${PX14400_INCLUDE_DIR}")
endif(PX14400_FOUND)

mark_as_advanced(PX14400_INCLUDE_DIRS PX14400_LIBRARIES)
