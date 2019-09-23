
#ifndef map_h
#define map_h


#include "../dataobj/koord.h"


class planquadrat_t;


/**
 * Stores all physical map properties like terrain, vehicles etc.
 */
class map_t
{
public:
	map_t();
	~map_t();

public:
	/**
	 * For performance reasons we have the map grid size cached locally, comes from the environment (Einstellungen)
	 * @brief Cached map grid size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 */
	koord cached_grid_size;

	/**
	 * For performance reasons we have the map size cached locally, comes from the environment (Einstellungen).
	 * @brief Cached map size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 * @note These values are one less than the size values of the grid.
	 */
	koord cached_size;

	/**
	 * @brief The maximum of the two dimensions.
	 * Maximum size for waiting bars etc.
	 */
	int cached_size_max;

	/**
	 * @name Map data structures
	 *       This variables represent the simulated map.
	 * @{
	 */

	/**
	 * Array containing all the map tiles.
	 * @see cached_size
	 */
	planquadrat_t *plan;

	/**
	 * Array representing the height of each point of the grid.
	 * @see map.cached_grid_size
	 */
	sint8 *grid_hgts;

	/**
	 * Array representing the height of water on each point of the grid.
	 * @see map.cached_grid_size
	 */
	sint8 *water_hgts;
	/** @} */

	/**
	 * Water level height.
	 * @author Hj. Malthaner
	 */
	sint8 groundwater;

	/**
	 * Current snow height.
	 * @note Might change during the game.
	 * @author prissi
	 */
	sint16 snowline;

	/**
	 * the maximum allowed world height.
	 */
	sint8 world_maximum_height;

	/**
	 * the minimum allowed world height.
	 */
	sint8 world_minimum_height;
};

#endif
