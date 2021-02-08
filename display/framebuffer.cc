/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "framebuffer.h"


framebuffer_t::framebuffer_t() :
	data(NULL),
	pitch(0),
	size(0, 0)
{
}


framebuffer_t::framebuffer_t(PIXVAL *data, size_t pitch, scr_size size) :
	data(data),
	pitch(pitch),
	size(size)
{
}
