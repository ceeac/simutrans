#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

find_package(Subversion)
find_package(Git)


if (Subversion_FOUND OR Git_FOUND)
	option(SIMUTRANS_WITH_REVISION "Build executable with SVN/git revision information" ON)
endif (Subversion_FOUND OR Git_FOUND)


# We have to try Git first because Subversion_WC_INFO does not fail silently if
# this repository is not a Subversion repository
if (GIT_FOUND)
	execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always HEAD
		OUTPUT_VARIABLE SIMUTRANS_WC_REVISION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (GIT_FOUND)

if (NOT SIMUTRANS_WC_REVISION AND Subversion_FOUND)
	Subversion_WC_INFO(${CMAKE_SOURCE_DIR} SIMUTRANS)
endif ()

if (SIMUTRANS_WC_REVISION)
	message(STATUS "Configuring Simutrans ${SIMUTRANS_WC_REVISION} ...")
else ()
	message(WARNING "Could not find revision information because this repository "
		"is neither a Subversion nor a Git repository. Revision information "
		"will be unavailable.")
endif ()
