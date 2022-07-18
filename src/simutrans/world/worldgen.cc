/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "worldgen.h"

#include "simcity.h"

#include "../builder/brueckenbauer.h"
#include "../builder/fabrikbauer.h"
#include "../builder/hausbauer.h"
#include "../builder/tree_builder.h"
#include "../builder/tunnelbauer.h"
#include "../builder/vehikelbauer.h"
#include "../builder/wegbauer.h"
#include "../dataobj/environment.h"
#include "../dataobj/route.h"
#include "../dataobj/translator.h"
#include "../descriptor/groundobj_desc.h"
#include "../display/viewport.h"
#include "../ground/boden.h"
#include "../ground/wasser.h"
#include "../gui/minimap.h"
#include "../obj/groundobj.h"
#include "../simloadingscreen.h"
#include "../simintr.h"
#include "../simmesg.h"
#include "../simsound.h"
#include "../sys/simsys.h"
#include "../utils/simrandom.h"
#include "../vehicle/vehicle.h"
#include "../vehicle/movingobj.h"


// for lake generation
static sint8 *stage;
static sint8 *new_stage;
static sint8 *local_stage;
static sint8 *max_water_hgt;
static bool need_to_flood;
static bool global_lake_fill;
static sint8 h;


worldgen_t::worldgen_t(karte_t *world, const settings_t *settings)
	: world(world)
	, sets(settings)
	, old_size(world->get_size())
	, new_size(settings->get_size_x(), settings->get_size_y())
	, new_world(old_size.x == 0 && old_size.y == 0)
{
}


sint32 worldgen_t::perlin_hoehe(const settings_t *const sets, koord k, koord const size)
{
	// replace the fixed values with your settings. Amplitude is the top highness of the mountains,
	// frequency is something like landscape 'roughness'; amplitude may not be greater than 160.0 !!!
	// please don't allow frequencies higher than 0.8, it'll break the AI's pathfinding.
	// Frequency values of 0.5 .. 0.7 seem to be ok, less is boring flat, more is too crumbled
	// the old defaults are given here: f=0.6, a=160.0
	switch( sets->get_rotation() ) {
		// 0: do nothing
		case 1: k = koord(k.y,size.x-k.x); break;
		case 2: k = koord(size.x-k.x,size.y-k.y); break;
		case 3: k = koord(size.y-k.y,k.x); break;
	}
//    double perlin_noise_2D(double x, double y, double persistence);
//    return ((int)(perlin_noise_2D(x, y, 0.6)*160.0)) & 0xFFFFFFF0;
	k = k + koord(sets->get_origin_x(), sets->get_origin_y());
	return ((int)(perlin_noise_2D(k.x, k.y, sets->get_map_roughness())*(double)sets->get_max_mountain_height())) / 16;
}


void worldgen_t::generate_terrain(loadingscreen_t &ls, const sint8 *h_field)
{
	planquadrat_t *new_plan = new planquadrat_t[new_size.x*new_size.y];
	sint8 *new_grid_hgts = new sint8[(new_size.x + 1) * (new_size.y + 1)];
	sint8 *new_water_hgts = new sint8[new_size.x * new_size.y];

	memset( new_grid_hgts, world->get_groundwater(), sizeof(sint8) * (new_size.x + 1) * (new_size.y + 1) );
	memset( new_water_hgts, world->get_groundwater(), sizeof(sint8) * new_size.x * new_size.y );

	const koord old_size = world->get_size();
	const bool new_world = old_size.x == 0 && old_size.y == 0;

	world->settings.set_size_x(new_size.x);
	world->settings.set_size_y(new_size.y);
	world->cached_grid_size = new_size;
	world->cached_size_max = max(world->cached_grid_size.x, world->cached_grid_size.y);
	world->cached_size.x = world->cached_grid_size.x-1;
	world->cached_size.y = world->cached_grid_size.y-1;

	if(  !new_world  ) {
		world->mute_sound(true);
		minimap_t::is_visible = false;

		if(is_display_init()) {
			display_show_pointer(false);
		}

// Copy old values:
		for (sint16 iy = 0; iy<old_size.y; iy++) {
			for (sint16 ix = 0; ix<old_size.x; ix++) {
				uint32 old_idx = ix + (iy*old_size.x);
				uint32 new_idx = ix + (iy*new_size.x);
				swap(new_plan[new_idx], world->plan[old_idx]);
				new_water_hgts[new_idx] = world->water_hgts[old_idx];
			}
		}
		for (sint16 iy = 0; iy<=old_size.y; iy++) {
			for (sint16 ix = 0; ix<=old_size.x; ix++) {
				uint32 old_idx = ix + (iy*(old_size.x+1));
				uint32 new_idx = ix + (iy*(new_size.x+1));
				new_grid_hgts[new_idx] = world->grid_hgts[old_idx];
			}
		}
	}

	delete [] world->plan;
	world->plan = new_plan;
	delete [] world->grid_hgts;
	world->grid_hgts = new_grid_hgts;
	delete [] world->water_hgts;
	world->water_hgts = new_water_hgts;

	if(  new_world  ) {
		// init max and min with defaults
		world->max_height = world->groundwater;
		world->min_height = world->groundwater;
	}

	setsimrand(0xFFFFFFFF, world->settings.get_map_number());
	clear_random_mode( 0xFFFF );
	set_random_mode( MAP_CREATE_RANDOM );

	if(  new_world  &&  !world->settings.heightfield.empty()  ) {
		// init from file
		for(int y=0; y<world->get_size().y; y++) {
			for(int x=0; x<world->get_size().x; x++) {
				world->grid_hgts[x + y*(world->get_size().x+1)] = h_field[x+(y*(sint32)world->get_size().x)]+1;
			}
			world->grid_hgts[world->get_size().x + y*(world->get_size().x+1)] = world->grid_hgts[world->get_size().x-1 + y*(world->get_size().x+1)];
		}

		// lower border
		memcpy( world->grid_hgts+(world->get_size().x+1)*(sint32)world->get_size().y, world->grid_hgts+(world->get_size().x+1)*(sint32)(world->get_size().y-1), world->get_size().x+1 );
		ls.set_progress(2);
	}
	else {
		if(  sets->get_rotation()==0  &&  sets->get_origin_x()==0  &&  sets->get_origin_y()==0) {
			// otherwise negative offsets may occur, so we cache only non-rotated maps
			init_perlin_map(new_size.x,new_size.y);
		}
		if (  !new_world  ) {
			// loop only new tiles:
			for(  sint16 y = 0;  y<=new_size.y;  y++  ) {
				for(  sint16 x = (y>old_size.y) ? 0 : old_size.x+1;  x<=new_size.x;  x++  ) {
					koord k(x,y);
					sint16 const h = this->perlin_hoehe(&world->settings, k, koord(old_size.x, old_size.y));
					world->set_grid_hgt_nocheck( k, (sint8) h);
				}
				ls.set_progress( (y*16)/new_size.y );
			}
		}
		else {
			world->world_xy_loop(this, &worldgen_t::perlin_hoehe_loop, karte_t::LOOP_GRIDS);
			ls.set_progress(2);
		}
		exit_perlin_map();
	}

	cleanup_map_grid();
}


void worldgen_t::generate_climates()
{
	// set climates in new area
	if( new_world ) {
		world->calc_climate_map_region( 0, 0, new_size.x, new_size.y );
	}
	else {
		world->calc_climate_map_region( 0, old_size.y, old_size.x, new_size.y );
		world->calc_climate_map_region( old_size.x, 0, new_size.x, new_size.y );
	}
}


void worldgen_t::generate_lakes()
{
	if(  new_world && sets->get_lakeheight() > world->get_groundwater()  ) {
		DBG_DEBUG("worldgen_t::generate_lakes()", "Generating lakes");
		create_lakes_internal( sets->get_lakeheight() );
	}
}


void worldgen_t::generate_rivers()
{
	// so at least some rivers end or start in lakes
	if(  env_t::river_types <= 0  ||  world->settings.get_river_number() <= 0  ) {
		return;
	}

	sint16 number = world->settings.get_river_number();

	DBG_DEBUG("worldgen_t::generate_rivers()", "Distributing rivers");

	// First check, whether there is a canal:
	const way_desc_t* river_desc = way_builder_t::get_desc( env_t::river_type[env_t::river_types-1], 0 );
	if(  river_desc == NULL  ) {
		// should never reaching here ...
		dbg->warning("worldgen_t::generate_rivers()", "There is no river defined!\n");
		return;
	}

	// create a vector of the highest points
	vector_tpl<koord> lake_tiles;
	vector_tpl<koord> sea_tiles;
	weighted_vector_tpl<koord> mountain_tiles;

	sint8 last_height = 1;
	koord last_koord(0,0);
	const sint16 max_dist = world->get_size().x+world->get_size().y;

	// trunk of 16 will ensure that rivers are long enough apart ...
	for(  sint16 y = 8;  y < world->get_size().y;  y+=16  ) {
		for(  sint16 x = 8;  x < world->get_size().x;  x+=16  ) {
			koord k(x,y);
			grund_t *gr = world->lookup_kartenboden_nocheck(k);
			const sint8 h = gr->get_hoehe() - world->get_water_hgt_nocheck(k);
			if(  gr->is_water()  ) {
				// may be good to start a river here
				if( gr->get_hoehe() <= world->get_groundwater() ) {
					sea_tiles.append(k);
				}
				else {
					lake_tiles.append(k);
				}
			}
			else if(  h>=last_height  ||  koord_distance(last_koord,k)>simrand(max_dist)  ) {
				// something worth to add here
				if(  h>last_height  ) {
					last_height = h;
				}
				last_koord = k;
				// using h*h as weight would give mountain sources more preferences
				// on the other hand most rivers do not string near summits ...
				mountain_tiles.append( k, h );
			}
		}
	}
	vector_tpl<koord> water_tiles( sea_tiles.empty() ? lake_tiles : sea_tiles );
	if (water_tiles.empty()) {
		dbg->message("worldgen_t::generate_rivers()", "There aren't any water tiles!");
		return;
	}

	// now make rivers
	sint16 retries = number*2;
	while(  number > 0  &&  !mountain_tiles.empty()  &&  retries>0  ) {
		// start with random coordinates
		koord const start = pick_any_weighted(mountain_tiles);
		mountain_tiles.remove( start );

		// build a list of matching targets
		vector_tpl<koord> valid_water_tiles;
		for(  uint32 i=0;  i<water_tiles.get_count();  i++  ) {
			const sint16 dist = koord_distance(start,water_tiles[i]);
			if(  world->settings.get_min_river_length() < dist  &&  dist < world->settings.get_max_river_length()  ) {
				valid_water_tiles.append( water_tiles[i] );
			}
		}

		// now try 256 random locations
		for(  sint32 i=0;  i<256  &&  !valid_water_tiles.empty();  i++  ) {
			koord const end = pick_any(valid_water_tiles);
			valid_water_tiles.remove( end );
			way_builder_t riverbuilder(world->get_public_player());
			riverbuilder.init_builder(way_builder_t::river, river_desc);
			sint16 dist = koord_distance(start,end);
			riverbuilder.set_maximum( dist*50 );
			riverbuilder.calc_route( world->lookup_kartenboden(end)->get_pos(), world->lookup_kartenboden(start)->get_pos() );
			if(  riverbuilder.get_count() >= (uint32)world->settings.get_min_river_length()  ) {
				// do not built too short rivers
				riverbuilder.build();
				number --;
				retries++;
				break;
			}
		}

		retries--;
	}

	// we gave up => tell the user
	if(  number>0  ) {
		dbg->warning( "worldgen_t::generate_rivers()", "Too many rivers requested! (%i not constructed)", number );
	}
}


void worldgen_t::generate_beaches()
{
	// bays have wide beaches
	for(  uint16 iy = 0;  iy < new_size.y;  iy++  ) {
		for(  uint16 ix = (iy >= old_size.y - 19) ? 0 : max( old_size.x - 19, 0 );  ix < new_size.x;  ix++  ) {
			grund_t *gr = world->lookup_kartenboden_nocheck(ix,iy);
			if(  gr->is_water()  &&  gr->get_hoehe()==world->groundwater  &&  gr->kann_alle_obj_entfernen(NULL)==NULL) {
				koord k( ix, iy );
				uint8 neighbour_water = 0;
				bool water[8] = {};
				sint16 total_ground = 0;
				// check whether nearby tiles are water
				for(  int i = 0;  i < 8;  i++  ) {
					if(  grund_t *gr2 = world->lookup_kartenboden( k + koord::neighbours[i] )  ) {
						total_ground++;
						if( gr2->hat_weg( water_wt ) ) {
							// never ever make a beach near a river mound
							neighbour_water = 8;
							break;
						}
						if(  gr2->is_water()  ) {
							water[i] = 1;
							neighbour_water++;
						}
					}
				}

				// make a count of nearby tiles - where tiles on opposite (+-1 direction) sides are water these count much more so we don't block straits
				for(  int i = 0;  i < 8;  i++  ) {
					if(  water[i]  ) {
						if(  water[(i + 3) & 7]  ||  water[(i + 4) & 7]  ||  water[(i + 5) & 7]  ) {
							neighbour_water++;
						}
					}
				}

				// if not much nearby water then turn into a beach
				if(  neighbour_water < (3*total_ground)/4  ) {
					world->set_water_hgt_nocheck( k, gr->get_hoehe() - 1 );
					world->raise_grid_to( ix, iy, gr->get_hoehe() );
					world->raise_grid_to( ix + 1, iy, gr->get_hoehe() );
					world->raise_grid_to( ix, iy + 1, gr->get_hoehe() );
					world->raise_grid_to( ix + 1, iy + 1 , gr->get_hoehe() );
					world->access_nocheck(k)->correct_water();
					world->access_nocheck(k)->set_climate( desert_climate );
				}
			}
		}
	}

	// headlands should not have beaches at all
	for(  uint16 iy = 0;  iy < new_size.y;  iy++  ) {
		for(  uint16 ix = (iy >= old_size.y - 19) ? 0 : max( old_size.x - 19, 0 );  ix < new_size.x;  ix++  ) {
			koord k( ix, iy );
			grund_t *gr = world->lookup_kartenboden_nocheck(k);
			if(  !gr->is_water()  &&  gr->get_pos().z == world->groundwater  ) {
				uint8 neighbour_water = 0;
				for(  int i = 0;  i < 8;  i++  ) {
					grund_t *gr2 = world->lookup_kartenboden( k + koord::neighbours[i] );
					if(  !gr2  ||  gr2->is_water()  ) {
						neighbour_water++;
					}
				}
				// if a lot of water nearby we are a headland
				if(  neighbour_water > 3  ) {
					world->access_nocheck(k)->set_climate( world->get_climate_at_height( world->groundwater + 1 ) );
				}
			}
		}
	}

	// remove any isolated 1 tile beaches
	for(  uint16 iy = 0;  iy < new_size.y;  iy++  ) {
		for(  uint16 ix = (iy >= old_size.y - 19) ? 0 : max( old_size.x - 19, 0 );  ix < new_size.x;  ix++  ) {
			koord k( ix, iy );
			if(  world->access_nocheck(k)->get_climate()  ==  desert_climate  ) {
				uint8 neighbour_beach = 0;
				//look up neighbouring climates
				climate neighbour_climate[8];
				for(  int i = 0;  i < 8;  i++  ) {
					koord k_neighbour = k + koord::neighbours[i];
					if(  !world->is_within_limits(k_neighbour)  ) {
						k_neighbour = world->get_closest_coordinate(k_neighbour);
					}
					neighbour_climate[i] = world->get_climate( k_neighbour );
				}

				// get transition climate - look for each corner in turn
				for( int i = 0;  i < 4;  i++  ) {
					climate transition_climate = (climate) max( max( neighbour_climate[(i * 2 + 1) & 7], neighbour_climate[(i * 2 + 3) & 7] ), neighbour_climate[(i * 2 + 2) & 7] );
					climate min_climate = (climate) min( min( neighbour_climate[(i * 2 + 1) & 7], neighbour_climate[(i * 2 + 3) & 7] ), neighbour_climate[(i * 2 + 2) & 7] );
					if(  min_climate <= desert_climate  &&  transition_climate == desert_climate  ) {
						neighbour_beach++;
					}
				}
				if(  neighbour_beach == 0  ) {
					world->access_nocheck(k)->set_climate( world->get_climate_at_height( world->groundwater + 1 ) );
				}
			}
		}
	}
}


void worldgen_t::generate_cities()
{
	sint32 new_city_count = sets->get_city_count();
	const sint32 new_mean_citizen_count = sets->get_mean_citizen_count();

	settings_t &settings = world->get_settings();

	DBG_DEBUG("worldgen_t::generate_cities()","prepare cities");
	vector_tpl<koord> *pos = stadt_t::random_place(new_city_count, old_size.x, old_size.y);

	if(  pos->empty()  ) {
		// could not generate any town
		delete pos;
		settings.set_city_count( world->stadt.get_count() ); // new number of towns (if we did not find enough positions)
		return;
	}

	const sint32 old_city_count = world->get_cities().get_count();
	new_city_count = pos->get_count();
	DBG_DEBUG("worldgen_t::generate_cities()", "Creating cities: %d", new_city_count);

	// if we could not generate enough positions ...
	settings.set_city_count(old_city_count);
	int old_progress = 16;

	loadingscreen_t ls( translator::translate( "distributing cities" ), 16 + 2 * (old_city_count + new_city_count) + 2 * new_city_count + (old_size.x == 0 ? settings.get_factory_count() : 0), true, true );

	{
		// Loop only new cities:
#ifdef DEBUG
		uint32 tbegin = dr_time();
#endif
		for(  int i=0;  i<new_city_count;  i++  ) {
			stadt_t* s = new stadt_t(world->get_public_player(), (*pos)[i], 1 );
			DBG_DEBUG("worldgen_t::generate_cities()", "Erzeuge stadt %i with %ld inhabitants", i, (s->get_city_history_month())[HIST_CITIZENS] );
			if (s->get_buildings() > 0) {
				world->add_city(s);
			}
			else {
				delete(s);
			}
		}

		// center on first city
		if(  old_size.x+old_size.y == 0  &&  world->get_cities().get_count()>0) {
			world->get_viewport()->change_world_position( world->get_cities()[0]->get_pos() );
		}

		delete pos;
#ifdef DEBUG
		dbg->message("worldgen_t::generate_cities()","took %lu ms for all towns", dr_time()-tbegin );
#endif

		uint32 game_start = world->get_current_month();
		// townhalls available since?
		for(building_desc_t const* const desc : *hausbauer_t::get_list(building_desc_t::townhall)) {
			uint32 intro_year_month = desc->get_intro_year_month();
			if(  intro_year_month<game_start  ) {
				game_start = intro_year_month;
			}
		}
		// streets since when?
		game_start = max( game_start, way_builder_t::get_earliest_way(road_wt)->get_intro_year_month() );

		uint32 original_start_year = world->get_current_month();
		uint32 original_industry_growth = settings.get_industry_increase_every();
		settings.set_industry_increase_every( 0 );

		for(  uint32 i=old_city_count;  i<world->get_cities().get_count();  i++  ) {
			// do final init after world was loaded/created
			world->get_cities()[i]->finish_rd();

//				int citizens=(int)(new_mean_citizen_count*0.9);
//				citizens = citizens/10+simrand(2*citizens+1);
			const sint32 citizens = (2500l * new_mean_citizen_count) /(simrand(20000)+100);

			sint32 diff = (original_start_year-game_start)/2;
			sint32 growth = 32;
			sint32 current_bev = 1;

			/* grow gradually while aging
				* the difference to the current end year will be halved,
				* while the growth step is doubled
				*/
			world->current_month = game_start;
			bool not_updated = false;
			bool new_town = true;

			while(  current_bev < citizens  ) {
				growth = min( citizens-current_bev, growth*2 );
				current_bev += growth;
				world->stadt[i]->change_size( growth, new_town );
				// Only "new" for the first change_size call
				new_town = false;
				if(  current_bev > citizens/2  &&  not_updated  ) {
					ls.set_progress( ++old_progress );
					not_updated = true;
				}
				world->current_month += diff;
				diff >>= 1;
			}

			// the growth is slow, so update here the progress bar
			ls.set_progress( ++old_progress );
		}

		world->current_month = original_start_year;
		settings.set_industry_increase_every( original_industry_growth );
		world->msg->clear();
	}

	world->finance_history_year[0][karte_t::WORLD_TOWNS]    = world->finance_history_month[0][karte_t::WORLD_TOWNS]    = world->stadt.get_count();
	world->finance_history_year[0][karte_t::WORLD_CITIZENS] = world->finance_history_month[0][karte_t::WORLD_CITIZENS] = world->last_month_bev;

	construct_intercity_roads(ls, old_city_count, new_city_count);
}


void worldgen_t::generate_trees()
{
	if( old_size.x==0 && old_size.y==0 ) {
		distribute_trees_region( 0, 0, new_size.x, new_size.y );
	}
	else {
		distribute_trees_region( 0, old_size.y, old_size.x, new_size.y );
		distribute_trees_region( old_size.x, 0, new_size.x, new_size.y );
	}
}


void worldgen_t::generate_groundobjs()
{
	if(  env_t::ground_object_probability <= 0  ) {
		return;
	}

	// add eyecandy like rocky, moles, flowers, ...
DBG_DEBUG("worldgen_t::generate_groundobjs()", "distributing groundobjs");

	sint32 queried = simrand(env_t::ground_object_probability*2-1);
	koord k;

	for(  k.y=0;  k.y<world->get_size().y;  k.y++  ) {
		for(  k.x=(k.y<old_size.y)?old_size.x:0;  k.x<world->get_size().x;  k.x++  ) {
			grund_t *gr = world->lookup_kartenboden_nocheck(k);
			if(  gr->get_typ()==grund_t::boden  &&  !gr->hat_wege()  ) {
				queried --;
				if(  queried<0  ) {
					// test for beach
					bool neighbour_water = false;
					for(int i=0; i<8; i++) {
						if(  world->is_within_limits(k + koord::neighbours[i])  &&  world->get_climate( k + koord::neighbours[i] ) == water_climate  ) {
							neighbour_water = true;
							break;
						}
					}

					const climate_bits cl = neighbour_water ? water_climate_bit : (climate_bits)(1<<world->get_climate(k));
					const groundobj_desc_t *desc = groundobj_t::random_groundobj_for_climate( cl, gr->get_grund_hang() );
					queried = simrand(env_t::ground_object_probability*2-1);

					if(desc) {
						gr->obj_add( new groundobj_t( gr->get_pos(), desc ) );
					}
				}
			}
		}
	}
}


void worldgen_t::generate_movingobjs()
{
	if(  env_t::moving_object_probability <= 0  ) {
		return;
	}

	// add animals and so on (must be done after growing and all other objects, that could change ground coordinates)
DBG_DEBUG("worldgen_t::generate_movingobjs()", "distributing movingobjs");

	const bool has_water = movingobj_t::random_movingobj_for_climate( water_climate )!=NULL;
	sint32 queried = simrand(env_t::moving_object_probability*2);
	koord k;

	// no need to test the borders, since they are mostly slopes anyway
	for(k.y=1; k.y<world->get_size().y-1; k.y++) {
		for(k.x=(k.y<old_size.y)?old_size.x:1; k.x<world->get_size().x-1; k.x++) {
			grund_t *gr = world->lookup_kartenboden_nocheck(k);
			// flat ground or open water
			if(  gr->get_top()==0  &&  (  (gr->get_typ()==grund_t::boden  &&  gr->get_grund_hang()==slope_t::flat)  ||  (has_water  &&  gr->is_water())  )  ) {
				queried --;
				if(  queried<0  ) {
					const groundobj_desc_t *desc = movingobj_t::random_movingobj_for_climate( world->get_climate(k) );
					if(  desc  &&  ( desc->get_waytype() != water_wt  ||  gr->get_hoehe() <= world->get_water_hgt_nocheck(k) )  ) {
						if(desc->get_speed()!=0) {
							queried = simrand(env_t::moving_object_probability*2);
							gr->obj_add( new movingobj_t( gr->get_pos(), desc ) );
						}
					}
				}
			}
		}
	}
}


void worldgen_t::generate_factories()
{
	dbg->message("karte_t::init()", "Creating factories ...");
	factory_builder_t::new_world();

	int consecutive_build_failures = 0;

	loadingscreen_t ls( translator::translate("distributing factories"), 16 + sets->get_city_count() * 4 + sets->get_factory_count(), true, true );

	while(  world->fab_list.get_count() < (uint32)sets->get_factory_count()  ) {
		if(  !factory_builder_t::increase_industry_density( false )  ) {
			if(  ++consecutive_build_failures > 3  ) {
				// Industry chain building starts failing consecutively as map approaches full.
				break;
			}
		}
		else {
			consecutive_build_failures = 0;
		}
		ls.set_progress( 16 + sets->get_city_count() * 4 + min(world->fab_list.get_count(), sets->get_factory_count()) );
	}
}


void worldgen_t::generate_attractions()
{
	factory_builder_t::distribute_attractions(sets->get_tourist_attractions());
}


void worldgen_t::perlin_hoehe_loop( sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max )
{
	koord k;
	for(  k.y = y_min;  k.y < y_max;  k.y++  ) {
		for(  k.x = x_min; k.x < x_max;  k.x++  ) {
			sint16 const h = perlin_hoehe(sets, k, koord(0, 0));
			world->set_grid_hgt_nocheck( k, (sint8) h);
		}
	}
}


void worldgen_t::cleanup_map_grid()
{
	/** @note First we'll copy the border heights to the adjacent tile.
	 * The best way I could find is raising the first new grid point to
	 * the same height the adjacent old grid point was and lowering to the
	 * same height again. This doesn't preserve the old area 100%, but it respects it
	 * somehow.
	 *
	 * This does not work for water tiles as for them get_hoehe will return the
	 * z-coordinate of the water surface, not the height of the underwater
	 * landscape.
	 */

	sint32 i;
	grund_t *gr;
	sint8 h;

	if (!new_world) {
		for(i=0; i<old_size.x; i++) {
			gr = world->lookup_kartenboden_nocheck(i, old_size.y-1);
			if (!gr->is_water()) {
				h = gr->get_hoehe(slope4_t::corner_SW);
				world->raise_grid_to(i, old_size.y+1, h);
				world->lower_grid_to(i, old_size.y+1, h );
			}
		}
		for(i=0; i<old_size.y; i++) {
			gr = world->lookup_kartenboden_nocheck(old_size.x-1, i);
			if (!gr->is_water()) {
				h = gr->get_hoehe(slope4_t::corner_NE);
				world->raise_grid_to(old_size.x+1, i, h);
				world->lower_grid_to(old_size.y+1, i, h);
			}
		}
		gr = world->lookup_kartenboden_nocheck(old_size.x-1, old_size.y -1);
		if (!gr->is_water()) {
			h = gr->get_hoehe(slope4_t::corner_SE);
			world->raise_grid_to(old_size.x+1, old_size.y+1, h);
			world->lower_grid_to(old_size.x+1, old_size.y+1, h);
		}
	}

	// smooth the new part, reassign slopes on new part
	cleanup_karte();
}


void worldgen_t::cleanup_karte()
{
	const int xoff = old_size.x;
	const int yoff = old_size.y;

	// we need a copy to smooth the map to a realistic level
	const sint32 grid_size = (world->get_size().x+1)*(sint32)(world->get_size().y+1);
	sint8 *grid_hgts_cpy = new sint8[grid_size];
	memcpy( grid_hgts_cpy, world->grid_hgts, grid_size );

	// the trick for smoothing is to raise each tile by one
	sint32 i,j;
	for(j=0; j<=world->get_size().y; j++) {
		for(i=j>=yoff?0:xoff; i<=world->get_size().x; i++) {
			world->raise_grid_to(i,j, grid_hgts_cpy[i+j*(world->get_size().x+1)] + 1);
		}
	}
	delete [] grid_hgts_cpy;

	// but to leave the map unchanged, we lower the height again
	for(j=0; j<=world->get_size().y; j++) {
		for(i=j>=yoff?0:xoff; i<=world->get_size().x; i++) {
			world->grid_hgts[i+j*(world->get_size().x+1)] --;
		}
	}

	if(  xoff==0 && yoff==0  ) {
		cleanup_grounds_loop( 0, world->get_size().x, 0, world->get_size().y );
	}
	else {
		cleanup_grounds_loop( 0, world->get_size().x, yoff, world->get_size().y );
		cleanup_grounds_loop( xoff, world->get_size().x, 0, yoff );
	}
}


void worldgen_t::cleanup_grounds_loop(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max)
{
	koord k;
	for(  k.y = y_min;  k.y < y_max;  k.y++  ) {
		for(  k.x = x_min; k.x < x_max;  k.x++  ) {
			planquadrat_t *pl = world->access_nocheck(k);
			slope_t::type slope = world->calc_natural_slope(k);
			sint8 height = world->min_hgt_nocheck(k);
			const sint8 water_hgt = world->get_water_hgt_nocheck(k);

			if(  height < water_hgt) {
				const sint8 disp_hn_sw = max( height + corner_sw(slope), water_hgt );
				const sint8 disp_hn_se = max( height + corner_se(slope), water_hgt );
				const sint8 disp_hn_ne = max( height + corner_ne(slope), water_hgt );
				const sint8 disp_hn_nw = max( height + corner_nw(slope), water_hgt );
				height = water_hgt;
				slope = encode_corners(disp_hn_sw - height, disp_hn_se - height, disp_hn_ne - height, disp_hn_nw - height);
			}

			if(  world->max_hgt_nocheck(k) <= water_hgt  ) {
				// create water
				pl->kartenboden_setzen( new wasser_t(koord3d( k, height)), true /* do not calc_image for water tiles */ );
			}
			else {
				// create ground
				pl->kartenboden_setzen( new boden_t(koord3d( k, height), slope ) );
			}

			if(  world->max_hgt_nocheck(k) > water_hgt  ) {
				world->set_water_hgt_nocheck(k, world->get_groundwater()-4);
			}
		}
	}
}


void worldgen_t::create_lakes_internal( sint8 max_lake_height )
{
	/* Lakes:
	 * For each height from groundwater+1 to max_lake_height we loop over
	 * all tiles in the map trying to increase water height to this value
	 * To start with every tile in the map is checked - but when we fail for
	 * a tile then it is excluded from subsequent checks
	 */
#define array_koord(px,py) (px + py * world->get_size().x)

	const uint16 size_x = world->get_size().x;
	const uint16 size_y = world->get_size().y;

	max_water_hgt = new sint8[size_x * size_y];
	for(  uint16 y = 0;  y < size_y;  y++  ) {
		for(  uint16 x = 0;  x < size_x;  x++  ) {
			const sint8 kartenboden_hgt = world->lookup_kartenboden_nocheck(x,y)->get_hoehe();
			const uint32 offset = array_koord(x,y);
			const sint8 water_hgt = world->water_hgts[offset]; // optimised <- get_water_hgt_nocheck(x, y);

			max_water_hgt[offset] = max(kartenboden_hgt, water_hgt);
		}
	}

	stage = new sint8[size_x * size_y];
	new_stage = new sint8[size_x * size_y];
	local_stage = new sint8[size_x * size_y];

	for(  h = world->get_groundwater()+1; h<max_lake_height; h++  ) {
		need_to_flood = false;
		memset( stage, -1, sizeof(sint8) * size_x * size_y );

// run over seams first to separate regions
		global_lake_fill = true;

		for(  int t = 1;  t < env_t::num_threads;  t++  ) {
			sint16 y_min = (t * size_y) / env_t::num_threads;
			this->create_lakes_loop( 0, size_x, y_min, y_min + 1 );
		}

		global_lake_fill = (env_t::num_threads == 1);

		world->world_xy_loop(this, &worldgen_t::create_lakes_loop);

		if (need_to_flood) {
			world->flood_to_depth(  h, stage  );
		}
		else {
			break;
		}
	}

	delete [] max_water_hgt;
	delete [] stage;
	delete [] new_stage;
	delete [] local_stage;

	for (planquadrat_t *pl = world->plan; pl < (world->plan + size_x * size_y); pl++) {
		pl->correct_water();
	}
}


void worldgen_t::create_lakes_loop(sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max)
{
	const uint16 size_x = world->get_size().x;
	const uint16 size_y = world->get_size().y;

	if(  x_min < 1  ) x_min = 1;
	if(  y_min < 1  ) y_min = 1;
	if(  x_max > size_x-1  ) x_max = size_x-1;
	if(  y_max > size_y-1  ) y_max = size_y-1;

	bool alternate_stage = false;      // false = new data in new_stage, true = new data in stage
	sint8 *this_stage = stage;         // local copy of pointer so we can swap round in thread
	sint8 *this_new_stage = new_stage; // local copy of pointer so we can swap round in thread

	for(  uint16 y = y_min;  y < y_max;  y++  ) {
		for(  uint16 x = x_min;  x < x_max;  x++  ) {
			uint32 offset = array_koord(x,y);
			if(  h<=max_water_hgt[offset]  ||  max_water_hgt[offset]==-126  ||  this_stage[offset]!=-1  ) {
				continue;
			}

			koord k(x,y);
			if(  global_lake_fill == true  ) {
				memcpy( this_new_stage, this_stage, sizeof(sint8) * size_x * size_y );
			}
			else {
				memcpy( this_new_stage + sizeof(sint8) * array_koord(0,y_min), this_stage + sizeof(sint8) * array_koord(0,y_min), sizeof(sint8) * size_x * (y_max-y_min) );
			}

			bool success = false;

			if(  global_lake_fill == true  ) {
				success = this->can_flood_to_depth(  k, h, this_new_stage, local_stage, 1, size_x-1, 1, size_y-1 );
			}
			else {
				success = this->can_flood_to_depth(  k, h, this_new_stage, local_stage, x_min, x_max, y_min, y_max );
			}

			if(  success  ) {
				if(  alternate_stage  ) {
					this_new_stage = new_stage;
					this_stage     = stage;
				}
				else {
					this_new_stage = stage;
					this_stage = new_stage;
				}

				alternate_stage = !alternate_stage;
				need_to_flood = true;
			}
			else {
				uint32 offset_end = array_koord(0,(global_lake_fill?size_y-1:y_max));
				for(  uint32 local_offset = array_koord(0,(global_lake_fill?1:y_min));  local_offset<offset_end;  local_offset++  ) {
					if(  local_stage[local_offset] > -1  ) {
						max_water_hgt[local_offset] = -126;
					}
				}
			}
		}
	}

	// ensure global stage reflect local data
	if(  alternate_stage  ) {
		if(  global_lake_fill == true  ) {
			std::swap(new_stage, stage);
		}
		else {
			memcpy( stage     + sizeof(sint8) * array_koord(0,y_min),
					new_stage + sizeof(sint8) * array_koord(0,y_min),
					sizeof(sint8) * size_x * (y_max-y_min) );
		}
	}
}


bool worldgen_t::can_flood_to_depth(koord k, sint8 new_water_height, sint8* stage, sint8* our_stage, sint16 x_min, sint16 x_max, sint16 y_min, sint16 y_max) const
{
	bool succeeded = true;
	if(  k == koord::invalid  ) {
		return false;
	}

	if(  new_water_height < world->get_groundwater() - 3  ) {
		return false;
	}

	// make a list of tiles to change
	// cannot use a recursive method as stack is not large enough!

	sint8 *from_dir = new sint8[world->get_size().x * world->get_size().y];
	bool local_stage = (our_stage==NULL);

	if(  local_stage  ) {
		our_stage = new sint8[world->get_size().x * world->get_size().y];
	}

	memset( our_stage + sizeof(sint8) * array_koord(0,y_min), -1, sizeof(sint8) * world->get_size().x * (y_max-y_min) );

	uint32 offset = array_koord(k.x,k.y);
	from_dir[offset]=-1;
	stage[offset]=0;
	our_stage[offset]=0;

	do {
		koord k_neighbour = k + koord::neighbours[our_stage[offset]];
		if(  k_neighbour.x < x_min  ||  k_neighbour.x>=x_max) {
			succeeded = false;
		}
		else if(  k_neighbour.y >= y_min  &&  k_neighbour.y<y_max  ) {
			const uint32 neighbour_offset = array_koord(k_neighbour.x,k_neighbour.y);

			// already visited
			if(our_stage[neighbour_offset] != -1) {
				goto next_neighbour;
			}

			// land or water height above
			if(max_water_hgt[neighbour_offset] >= new_water_height) {
				goto next_neighbour;
			}

			//move on to next tile
			from_dir[neighbour_offset] = our_stage[offset];
			stage[neighbour_offset] = 0;
			our_stage[neighbour_offset] = 0;
			k = k_neighbour;
			offset = neighbour_offset;
			continue;
		}
		else if(  succeeded  &&  (global_lake_fill  ||  k_neighbour.y < 1  ||  k_neighbour.y>=world->get_size().y-1)  ) {
			// edge of map - we keep iterating so we can mark all connected tiles as failing
			succeeded = false;
		}

	next_neighbour:
		//return back to previous tile
		while(  our_stage[offset]==7  ) {
			if(  from_dir[offset] == -1  ) goto exit_iteration;    // for natural maps only need to have checked one path out from original tile
			k = k - koord::neighbours[from_dir[offset]];
			offset = array_koord(k.x,k.y);
		}
		our_stage[offset]++;
	} while(  true  );

exit_iteration:

	delete [] from_dir;

	if(  local_stage  ) {
		delete [] our_stage;
	}

	return succeeded;
}


void worldgen_t::distribute_trees_region(sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom)
{
	// now distribute trees
	DBG_DEBUG("worldgen_t::distribute_trees_region()","distributing trees");

	switch (sets->get_tree_distribution()) {
	case settings_t::TREE_DIST_RAINFALL:
		if( world->humidity_map.get_height() != 0 ) {
			koord pos;
			for(  pos.y=ytop;  pos.y<ybottom;  pos.y++  ) {
				for(  pos.x=xtop;  pos.x<xbottom;  pos.x++  ) {
					grund_t *gr = world->lookup_kartenboden(pos);
					if(gr->get_top() == 0  &&  gr->get_typ() == grund_t::boden)  {
						if(world->humidity_map.at(pos.x,pos.y)>75) {
							const uint32 tree_probability = (world->humidity_map.at(pos.x,pos.y) - 75)/5 + 38;
							uint8 number_to_plant = 0;
							uint8 const max_trees_here = min(world->get_settings().get_max_no_of_trees_on_square(), (tree_probability - 38 + 1) / 2);

							for (uint8 c2 = 0 ; c2<max_trees_here; c2++) {
								const uint32 rating = simrand(10) + 38 + c2*2;
								if (rating < tree_probability ) {
									number_to_plant++;
								}
							}

							tree_builder_t::plant_tree_on_coordinate(pos, world->get_settings().get_max_no_of_trees_on_square(), number_to_plant);
						}
						else if(world->humidity_map.at(pos.x,pos.y)>75) {
							// plant spare trees, (those with low preffered density) or in an entirely tree climate
							uint16 cl = 1 << world->get_climate(pos);
							settings_t const& s = world->get_settings();
							if ((cl & s.get_no_tree_climates()) == 0 && ((cl & s.get_tree_climates()) != 0 || simrand(s.get_forest_inverse_spare_tree_density() * /*dichte*/3) < 100)) {
								tree_builder_t::plant_tree_on_coordinate(pos, 1, 1);
							}
						}
					}
				}
			}
			break;
		}
		// fall-through
	case settings_t::TREE_DIST_RANDOM:
		// no humidity data or on request
		tree_builder_t::distribute_trees(3, xtop, ytop, xbottom, ybottom );
		break;
	case settings_t::TREE_DIST_NONE:
		// no trees
		break;
	case settings_t::TREE_DIST_COUNT:
		assert(false); // should not happen
		break;
	}
}


void worldgen_t::construct_intercity_roads(loadingscreen_t &ls, sint32 old_city_count, sint32 new_city_count)
{
	settings_t &settings = world->get_settings();

	// connect some cities with roads
	way_desc_t const* desc = settings.get_intercity_road_type(world->get_timeline_year_month());
	if(desc == NULL) {
		// try some default (might happen with timeline ... )
		desc = way_builder_t::weg_search(road_wt,80,world->get_timeline_year_month(),type_flat);
	}

	way_builder_t bauigel(world->get_public_player());
	bauigel.init_builder(way_builder_t::strasse | way_builder_t::terraform_flag, desc, tunnel_builder_t::get_tunnel_desc(road_wt, 15, world->get_timeline_year_month()), bridge_builder_t::find_bridge(road_wt, 15, world->get_timeline_year_month()) );
	bauigel.set_keep_existing_ways(true);
	bauigel.set_maximum(env_t::intercity_road_length);

	// **** intercity road construction
	int count = 0;
	sint32 const n_cities  = settings.get_city_count();
	int    const max_count = n_cities * (n_cities - 1) / 2 - old_city_count * (old_city_count - 1) / 2;

	// something to do??
	if(  max_count <= 0  ) {
		return;
	}

	// print("Building intercity roads ...\n");
	ls.set_max( 16 + 2 * (old_city_count + new_city_count) + 2 * new_city_count + (old_size.x == 0 ? settings.get_factory_count() : 0) );

	// find townhall of city i and road in front of it
	const vector_tpl<koord3d> townhall_road_positions = find_townhall_road_positions();

	// compute all distances
	uint8 conn_comp=1; // current connection component for phase 0
	vector_tpl<uint8> city_flag; // city already connected to the graph? >0 nr of connection component
	array2d_tpl<sint32> city_dist(settings.get_city_count(), settings.get_city_count());
	for (sint32 i = 0; i < settings.get_city_count(); ++i) {
		city_dist.at(i,i) = 0;
		for (sint32 j = i + 1; j < settings.get_city_count(); ++j) {
			city_dist.at(i,j) = koord_distance(townhall_road_positions[i], townhall_road_positions[j]);
			city_dist.at(j,i) = city_dist.at(i,j);
			// count unbuildable connections to new cities
			if(  j>=old_city_count && city_dist.at(i,j) >= env_t::intercity_road_length  ) {
				count++;
			}
		}
		city_flag.append( i < old_city_count ? conn_comp : 0 );

		// progress bar stuff
		ls.set_progress( 16 + 2 * new_city_count + count * settings.get_city_count() * 2 / max_count );
	}
	// mark first town as connected
	if (old_city_count==0) {
		city_flag[0]=conn_comp;
	}

	// get a default vehikel
	route_t verbindung;
	vehicle_desc_t test_drive_desc(road_wt, 500, vehicle_desc_t::diesel );
	vehicle_t *test_driver = vehicle_builder_t::build(koord3d(), world->get_public_player(), NULL, &test_drive_desc);
	test_driver->set_flag( obj_t::not_on_map );

	bool ready=false;
	uint8 phase=0;
	// 0 - first phase: build minimum spanning tree (edge weights: city distance)
	// 1 - second phase: try to complete the graph, avoid edges that
	// == have similar length as an already existing connection
	// == lead to triangles with an angle >90 deg

	while( phase < 2  ) {
		ready = true;
		koord conn = koord::invalid;
		sint32 best = env_t::intercity_road_length;

		if(  phase == 0  ) {
			// loop over all unconnected cities
			for (int i = 0; i < settings.get_city_count(); ++i) {
				if(  city_flag[i] == conn_comp  ) {
					// loop over all connections to connected cities
					for (int j = old_city_count; j < settings.get_city_count(); ++j) {
						if(  city_flag[j] == 0  ) {
							ready=false;
							if(  city_dist.at(i,j) < best  ) {
								best = city_dist.at(i,j);
								conn = koord(i,j);
							}
						}
					}
				}
			}
			// did we completed a connection component?
			if(  !ready  &&  best == env_t::intercity_road_length  ) {
				// next component
				conn_comp++;
				// try the first not connected city
				ready = true;
				for(  int i = old_city_count;  i < settings.get_city_count();  ++i  ) {
					if(  city_flag[i] ==0  ) {
						city_flag[i] = conn_comp;
						ready = false;
						break;
					}
				}
			}
		}
		else {
			// loop over all unconnected cities
			for (int i = 0; i < settings.get_city_count(); ++i) {
				for (int j = max(old_city_count, i + 1);  j < settings.get_city_count(); ++j) {
					if(  city_dist.at(i,j) < best  &&  city_flag[i] == city_flag[j]  ) {
						bool ok = true;
						// is there a connection i..l..j ? forbid stumpfe winkel
						for (int l = 0; l < settings.get_city_count(); ++l) {
							if(  city_flag[i] == city_flag[l]  &&  city_dist.at(i,l) == env_t::intercity_road_length  &&  city_dist.at(j,l) == env_t::intercity_road_length  ) {
								// cosine < 0 ?
								koord3d d1 = townhall_road_positions[i]-townhall_road_positions[l];
								koord3d d2 = townhall_road_positions[j]-townhall_road_positions[l];
								if(  d1.x*d2.x + d1.y*d2.y < 0  ) {
									city_dist.at(i,j) = env_t::intercity_road_length+1;
									city_dist.at(j,i) = env_t::intercity_road_length+1;
									ok = false;
									count ++;
									break;
								}
							}
						}
						if(ok) {
							ready = false;
							best = city_dist.at(i,j);
							conn = koord(i,j);
						}
					}
				}
			}
		}
		// valid connection?
		if(  conn.x >= 0  ) {
			// is there a connection already
			const bool connected = (  phase==1  &&  verbindung.calc_route( world, townhall_road_positions[conn.x], townhall_road_positions[conn.y], test_driver, 0, 0 )  );
			// build this connection?
			bool build = false;
			// set appropriate max length for way builder
			if(  connected  ) {
				if(  2*verbindung.get_count() > (uint32)city_dist.at(conn)  ) {
					bauigel.set_maximum(verbindung.get_count() / 2);
					build = true;
				}
			}
			else {
				bauigel.set_maximum(env_t::intercity_road_length);
				build = true;
			}

			if(  build  ) {
				bauigel.calc_route(townhall_road_positions[conn.x],townhall_road_positions[conn.y]);
			}

			if(  build  &&  bauigel.get_count() >= 2  ) {
				bauigel.build();
				if (phase==0) {
					city_flag[ conn.y ] = conn_comp;
				}
				// mark as built
				city_dist.at(conn) =  env_t::intercity_road_length;
				city_dist.at(conn.y, conn.x) =  env_t::intercity_road_length;
				count ++;
			}
			else {
				// do not try again
				city_dist.at(conn) =  env_t::intercity_road_length+1;
				city_dist.at(conn.y, conn.x) =  env_t::intercity_road_length+1;
				count ++;

				if(  phase == 0  ) {
					// do not try to connect to this connected component again
					for(  int i = 0;  i < settings.get_city_count();  ++i  ) {
						if(  city_flag[i] == conn_comp  && city_dist.at(i, conn.y)<env_t::intercity_road_length) {
							city_dist.at(i, conn.y) =  env_t::intercity_road_length+1;
							city_dist.at(conn.y, i) =  env_t::intercity_road_length+1;
							count++;
						}
					}
				}
			}
		}

		// progress bar stuff
		ls.set_progress( 16 + 2 * new_city_count + count * settings.get_city_count() * 2 / max_count );

		// next phase?
		if(ready) {
			phase++;
			ready = false;
		}
	}

	delete test_driver;
}


vector_tpl<koord3d> worldgen_t::find_townhall_road_positions()
{
	const settings_t &settings = world->get_settings();
	vector_tpl<koord3d> townhall_road_positions;

	for (int i = 0;  i < settings.get_city_count(); ++i) {
		koord k1(world->stadt[i]->get_townhall_road());
		if (world->lookup_kartenboden(k1)  &&  world->lookup_kartenboden(k1)->hat_weg(road_wt)) {
			townhall_road_positions.append(world->lookup_kartenboden(k1)->get_pos());
			continue; // found;
		}

		// look for a road near the townhall
		gebaeude_t const* const gb = obj_cast<gebaeude_t>(world->lookup_kartenboden(world->stadt[i]->get_pos())->first_obj());
		bool ok = false;

		if(  gb  &&  gb->is_townhall()  ) {
			koord k_check = world->stadt[i]->get_pos() + koord(-1,-1);
			const koord size = gb->get_tile()->get_desc()->get_size(gb->get_tile()->get_layout());
			koord inc(1,0);

			// scan all adjacent tiles, take the first that has a road
			for(sint32 i=0; i<2*size.x+2*size.y+4  &&  !ok; i++) {
				grund_t *gr = world->lookup_kartenboden(k_check);
				if (gr  &&  gr->hat_weg(road_wt)) {
					townhall_road_positions.append(gr->get_pos());
					ok = true;
				}
				k_check = k_check + inc;
				if (i==size.x+1) {
					inc = koord(0,1);
				}
				else if (i==size.x+size.y+2) {
					inc = koord(-1,0);
				}
				else if (i==2*size.x+size.y+3) {
					inc = koord(0,-1);
				}
			}
		}

		if (!ok) {
			townhall_road_positions.append( koord3d::invalid );
		}
	}

	return townhall_road_positions;
}
