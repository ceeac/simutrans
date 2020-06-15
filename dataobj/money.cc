/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "money.h"

#include "../dataobj/loadsave.h"


money_t::money_t(sint64 amount)
	: value(amount)
{
}


money_t::money_t(sint64 credits, sint64 cents)
	: value(credits * 100 + cents)
{
}


#ifndef MAKEOBJ
void money_t::rdwr(loadsave_t* file)
{
	file->rdwr_longlong(value);
}
#endif
