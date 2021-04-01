/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_FRAMEBUFFER_H
#define DISPLAY_FRAMEBUFFER_H


#include "simgraph.h"


/// Provides access to the screen buffer texture.
struct framebuffer_t
{
public:
	scr_coord_val pitch; ///< Number of pixels per row
	scr_size size;       ///< Size of visible area (size.w <= pitch)
	PIXVAL *textur;      ///< Pointer to content

public:
	framebuffer_t();
	framebuffer_t(scr_size size, scr_coord_val pitch, PIXVAL *textur);

public:
	bool is_valid() const { return pitch > 0; }

	      PIXVAL *access_pixel(scr_coord_val x, scr_coord_val y)       { return textur + x + y*pitch; }
	const PIXVAL *access_pixel(scr_coord_val x, scr_coord_val y) const { return textur + x + y*pitch; }
};


#endif
