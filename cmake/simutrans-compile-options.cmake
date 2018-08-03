#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

include(CheckCXXCompilerFlag)


#
# This function adds all supported compiler flags to result_list
# Example: SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(COMMON_COMPILE_OPTIONS -Wall -Wextra -Werror)
# will add -Wall -Wextra -Werror to COMMON_COMPILE_OPTIONS.
#
function(SIMUTRANS_CHECK_CXX_COMPILER_FLAGS result_list)
	set(temp_list "")
	foreach (flag ${ARGN})
		# We cannot check for -Wno-* or -fno-* as this won't throw a warning so we must check for -W* or -f* directly
		string(REGEX REPLACE "^(-[Wf])no-" "\\1" sanitizedFlag ${flag})
		set(cachedVarName ${sanitizedFlag})
		string(REPLACE "+" "X" cachedVarName ${cachedVarName})
		string(REGEX REPLACE "[-=]" "_" cachedVarName ${cachedVarName})

		if (NOT ${CMAKE_CXX_COMPILER_ID}_${cachedVarName}_CHECKED)
			CHECK_CXX_COMPILER_FLAG(${sanitizedFlag} CXX_FLAG_${cachedVarName}_SUPPORTED)
			set(${CMAKE_CXX_COMPILER_ID}_${cachedVarName}_CHECKED YES CACHE INTERNAL "")
		endif()

		if (CXX_FLAG_${cachedVarName}_SUPPORTED)
			list(APPEND temp_list ${flag})
		endif (CXX_FLAG_${cachedVarName}_SUPPORTED)

		unset(cachedVarName)
		unset(sanitizedFlag)
	endforeach ()

	if (NOT ${result_list})
		set(${result_list} ${temp_list} PARENT_SCOPE)
	elseif (temp_list)
		set(${result_list} "${${result_list}};${temp_list}" PARENT_SCOPE)
	endif ()
endfunction()


SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
	-Wall
	-Wextra
	-Wformat=2
	-Wundef
	-Wmissing-include-dirs
	-Wcast-qual
	-Wpointer-arith
	-Wcast-align
	-Walloca
	-Wduplicated-cond
	-Wno-implicit-fallthrough
	-Wno-format-nonliteral
	-Wno-unused-result
	-Wno-array-bounds
	-Wno-strict-aliasing
	-Wno-overloaded-virtual
	-Wno-cpp # Squelch #warning directive that is already emitted at CMake configure time
	-Wno-invalid-source-encoding
	-Wno-cast-align
	-Wno-address-of-packed-member
	-Wno-undef
	-Wno-char-subscripts
	-Wno-unused-function
	-Wno-narrowing
	-Wno-int-in-bool-context
	-Wno-deprecated-copy
	-Wno-c++11-narrowing
)

if (SIMUTRANS_PROFILE)
	SIMUTRANS_CHECK_CXX_COMPILER_FLAGS(SIMUTRANS_COMMON_COMPILE_OPTIONS
		-pg -fno-inline -fno-schedule-insns
	)
endif (SIMUTRANS_PROFILE)
