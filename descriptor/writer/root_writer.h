/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef ROOT_WRITER_H
#define ROOT_WRITER_H

#include <string>
#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"

struct obj_node_info_t;


class root_writer_t : public obj_writer_t {
	private:
		static root_writer_t the_instance;

		static std::string inpath;

		root_writer_t() { register_writer(false); }

		void copy_nodes(FILE* outfp, FILE* infp, obj_node_info_t& info);
		void write_header(FILE* fp);
		void write_obj_node_info_t(FILE* outfp, const obj_node_info_t &root);

	public:
		void capabilites();
		static root_writer_t* instance() { return &the_instance; }
		virtual obj_type get_type() const { return obj_root; }
		virtual const char* get_type_name() const { return "root"; }

		void write(const char* name, int argc, char* argv[]);
		void dump(int argc, char* argv[]);
		void list(int argc, char* argv[]);
		void copy(const char* name, int argc, char* argv[]);

		/* makes single files from a merged file */
		void uncopy(const char* name);

		static const std::string & get_inpath() { return inpath; }

	private:
		bool do_copy(FILE* outfp, obj_node_info_t& root, const char* open_file_name);
		bool do_dump(const char* open_file_name);
		bool do_list(const char* open_file_name);
};

#endif
