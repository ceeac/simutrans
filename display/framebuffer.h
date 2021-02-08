/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_FRAMEBUFFER_H
#define UTILS_FRAMEBUFFER_H


#include "simgraph.h"


class framebuffer_t
{
public:
	framebuffer_t();
	framebuffer_t(PIXVAL *data, size_t pitch, scr_size size);

public:
	const PIXVAL *access_pixel(scr_coord_val x, scr_coord_val y) const { return data + x + y*pitch; }
	PIXVAL *      access_pixel(scr_coord_val x, scr_coord_val y)       { return data + x + y*pitch; }

	bool is_valid() const { return pitch > 0; }

public:
	PIXVAL *data;
	scr_coord_val pitch;
	scr_size size;
};


#endif
