/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef __TEXT_BESCH_H
#define __TEXT_BESCH_H

#include "obj_desc.h"


/*
 *  Autor:
 *      Volker Meyer
 */
class text_desc_t : public obj_desc_t {
public:
		const char* get_text() const { return text; }

		using obj_desc_t::operator new;

	private:
		char text[];

	friend class text_reader_t;
};

#endif
