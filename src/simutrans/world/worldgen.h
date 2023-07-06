/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef WORLD_WORLDGEN_H
#define WORLD_WORLDGEN_H


#include "../dataobj/koord.h"
#include "../tpl/vector_tpl.h"


class karte_t;
class settings_t;
class loadingscreen_t;


/// Handles map generation/enlargement
class worldgen_t
{
	karte_t *world;

	const settings_t *sets;
	const koord old_size;
	const koord new_size;
	const bool new_world;

public:
	/// @param overwrite_existing true to overwrite data in existing world (old_size), used while importing old save files
	worldgen_t(karte_t *world, const settings_t *settings, bool overwrite_existing = false);

public:
	/**
	 * Height of a point of the map with "perlin noise".
	 * Uses map roughness and mountain height from @p sets.
	 */
	static sint32 perlin_hoehe(const settings_t *sets, koord k, koord size);

	/// @{
	/// @ingroup worldgen-functions
	/// These functions are called to generate a new world or enlarge an existing world.
	/// They are called from top to bottom, see @ref karte_t::init_or_enlarge_map.
public:
	void generate_terrain(loadingscreen_t &ls, const sint8 *h_field);
	void generate_climates();
	void generate_lakes();
	void generate_rivers();
	void generate_beaches();
	void generate_cities();
	void generate_trees();
	void generate_groundobjs();
	void generate_movingobjs();
	void generate_factories();
	void generate_attractions();
	/// @}

private:
	/// Loops over tiles setting heights from perlin noise
	void perlin_hoehe_loop(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max);

	void cleanup_map_grid();
	void cleanup_karte();
	void cleanup_grounds_loop(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max);

	// climates
	void calc_climate_map_region(sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);
	void assign_climate_map_region(sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

	/// Calculates appropriate climate for a region using elliptic areas for each
	void calc_humidity_map_region(sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

	/// Will create lakes (multithreaded).
	void create_lakes_internal(sint8 height);
	void create_lakes_loop(sint16 xmin, sint16 xmax, sint16 ymin, sint16 ymax);
	bool can_flood_to_depth(koord pos, sint8 new_water_height, sint8 *stage, sint8 *our_stage, sint16, sint16, sint16, sint16) const;

	void distribute_trees_region(sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

	void construct_intercity_roads(loadingscreen_t &ls, sint32 old_city_count, sint32 new_city_count);

	vector_tpl<koord3d> find_townhall_road_positions();
};


#endif
