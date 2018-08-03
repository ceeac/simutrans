#
# This file is part of the Simutrans project under the artistic licence.
# (see licence.txt)
#

find_package(Allegro)
find_package(SDL 1.2)
find_package(SDL_mixer 1.2)
find_package(SDL2)


option(SIMUTRANS_BUILD_HEADLESS "Build Simutrans without graphics backend. Recommended for standalone servers." OFF)

if (NOT SIMUTRANS_BUILD_HEADLESS)
	if (SDL2_FOUND)
		list(APPEND AVAILABLE_BACKENDS "sdl2")
		mark_as_advanced(SDL2_DIR)
	endif (SDL2_FOUND)

	if (SDL_FOUND)
		list(APPEND AVAILABLE_BACKENDS "sdl")
		mark_as_advanced(SDLMAIN_LIBRARY SDL_INCLUDE_DIR SDL_LIBRARY)
		if (SDL_MIXER_FOUND)
			list(APPEND AVAILABLE_BACKENDS "mixer_sdl")
		endif (SDL_MIXER_FOUND)

	endif (SDL_FOUND)

	if (Allegro_FOUND)
		list(APPEND AVAILABLE_BACKENDS "allegro")
	endif (Allegro_FOUND)

	if (AVAILABLE_BACKENDS)
		string(REGEX MATCH "^[^;][^;]*" FIRST_BACKEND "${AVAILABLE_BACKENDS}")
		set(SIMUTRANS_BACKEND "${FIRST_BACKEND}" CACHE STRING "Graphics backend")
		set_property(CACHE SIMUTRANS_BACKEND PROPERTY STRINGS ${AVAILABLE_BACKENDS})
	else (AVAILABLE_BACKENDS)
		message(WARNING "No suitable backend found (Must be one of: allegro sdl mixer_sdl sdl2).\nFalling back to headless compilation.")
		set(SIMUTRANS_BUILD_HEADLESS TRUE)
	endif (AVAILABLE_BACKENDS)
endif (NOT SIMUTRANS_BUILD_HEADLESS)
