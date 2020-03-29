/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IMAGELIST3D_WRITER_H
#define IMAGELIST3D_WRITER_H

#include <string>
#include "obj_writer.h"
#include "../objversion.h"


template<class T> class slist_tpl;


class imagelist3d_writer_t : public obj_writer_t
{
	private:
		static imagelist3d_writer_t the_instance;

		imagelist3d_writer_t() { register_writer(false); }

	public:
		static imagelist3d_writer_t* instance() { return &the_instance; }

		virtual obj_type get_type() const { return obj_imagelist3d; }
		virtual const char* get_type_name() const { return "imagelist3d"; }

		void write_obj(FILE* fp, obj_node_t& parent, const slist_tpl<slist_tpl<slist_tpl<std::string> > >& keys);
};

#endif
