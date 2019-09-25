/**
 * Copyright (c) 2019 Simutrans contributors
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */
#include "map.h"

#include "../macros.h"


map_t::map_t() :
	cached_size_max(0),
	plan(NULL),
	grid_hgts(NULL),
	water_hgts(NULL),
	groundwater(0),
	snowline(0),
	max_allowed_height(0),
	min_allowed_height(0),
	max_height(0),
	min_height(0),
	convoys(0),
	attractions(16),
	cities(0)
{
	MEMZERON(height_to_climate, MAP_HEIGHTTOCLIMATE_SIZE);
}


map_t::~map_t()
{
}
