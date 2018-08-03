#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#
#
# - Find CCache compiler cache executable.
#
# This module defines the following variables:
#   CCache_FOUND        - true if CCache was found.
#   CCache_EXECUTABLE   - Path to CCache executable.
#   CCache_VERSION      - Version string of CCache executable.
#

include(FindPackageHandleStandardArgs)

find_program(CCache_EXECUTABLE ccache)

if (CCache_EXECUTABLE)
	execute_process(COMMAND "${CCache_EXECUTABLE}" --version
		OUTPUT_VARIABLE CCache_VERSION_OUTPUT
	)

	string(REGEX MATCH "version ([0-9]+\\.[0-9]+\\.[0-9]+)" CCache_VERSION_TEMP ${CCache_VERSION_OUTPUT})
	set(CCache_VERSION "${CMAKE_MATCH_0}")
endif (CCache_EXECUTABLE)


find_package_handle_standard_args(CCache
	FOUND_VAR     CCache_FOUND
	REQUIRED_VARS CCache_EXECUTABLE
	VERSION_VAR   CCache_VERSION
)

if (CCache_FOUND OR NOT CCache_FIND_REQUIRED)
	mark_as_advanced(CCache_EXECUTABLE)
endif ()
