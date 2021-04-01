/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "framebuffer.h"


framebuffer_t::framebuffer_t() :
	pitch(0),
	size(scr_size::invalid),
	textur(NULL)
{
}


framebuffer_t::framebuffer_t(scr_size size, scr_coord_val pitch, PIXVAL *textur)  :
	pitch(pitch),
	size(size),
	textur(textur)
{
	assert(pitch > 0);
	assert(size.w <= pitch);
}
