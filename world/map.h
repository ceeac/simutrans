
#ifndef map_h
#define map_h


#include "../dataobj/koord.h"


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
};

#endif
