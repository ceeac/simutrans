
#ifndef map_h
#define map_h


#include "../dataobj/koord.h"

#include "../tpl/slist_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/weighted_vector_tpl.h"

#include "../convoihandle_t.h"

#include "sync_list.h"


class fabrik_t;
class gebaeude_t;
class planquadrat_t;
class stadt_t;


/**
 * Stores all physical map properties like terrain, vehicles etc.
 */
class map_t
{
	friend class karte_t;

public:
	map_t();
	~map_t();

private:
	/**
	 * For performance reasons we have the map grid size cached locally, comes from the environment (env_t)
	 * @brief Cached map grid size.
	 * @note Valid coords are (0..x-1,0..y-1)
	 */
	koord cached_grid_size;

	/**
	 * For performance reasons we have the map size cached locally, comes from the environment (env_t).
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

	/// the minimum/maximum allowed world height.
	sint8 max_allowed_height;
	sint8 min_allowed_height;

	/// cache the current maximum and minimum height on the map
	/// @sa karte_t::update_cached_minmax_height
	sint8 max_height, min_height;


	/// @name Map objects (vehicles etc.)
	/// @{

	/// Array containing the convois.
	vector_tpl<convoihandle_t> convoys;

	/// Array containing all factories on the map.
	slist_tpl<fabrik_t *> fab_list;

	weighted_vector_tpl<gebaeude_t *> attractions;

	/// Stores the cities.
	weighted_vector_tpl<stadt_t*> cities;

	sync_list_t sync;              ///< vehicles, transformers, traffic lights
	sync_list_t sync_eyecandy;     ///< animated buildings
	sync_list_t sync_way_eyecandy; ///< smoke

	/// @}
};

#endif
