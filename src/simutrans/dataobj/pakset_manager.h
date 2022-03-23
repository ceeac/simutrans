/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_PAKSET_MANAGER_H
#define DATAOBJ_PAKSET_MANAGER_H


#include "../descriptor/objversion.h"
#include "../tpl/inthashtable_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/ptrhashtable_tpl.h"


class obj_desc_t;
class obj_reader_t;


class pakset_manager_t
{
	/// table of registered obj readers sorted by id
	typedef inthashtable_tpl<obj_type, obj_reader_t *> obj_map;
	static obj_map* obj_reader;

	//
	// object addresses needed for resolving xrefs later
	// - stored in a hash table with type and name
	//
	static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t *> > loaded;
	typedef inthashtable_tpl<obj_type, stringhashtable_tpl<slist_tpl<obj_desc_t**> > > unresolved_map;
	static unresolved_map unresolved;
	static ptrhashtable_tpl<obj_desc_t **, int>  fatals;

	/// Read a descriptor node.
	/// @param fp File to read from
	/// @param[out] data If reading is successful, contains descriptor for the object, else NULL.
	/// @param register_nodes Nesting level for desc-nodes, should normally be 0
	/// @param version File format version
	static bool read_nodes(FILE* fp, obj_desc_t *&data, int register_nodes, uint32 version);
	static bool skip_nodes(FILE *fp, uint32 version);

public:
	static void register_reader(obj_reader_t *reader);

	/**
	 * Loads all pak files from a directory, displaying a progress bar if the display is initialized
	 * @param path Directory to be scanned for PAK files
	 * @param message Label to show over the progress bar
	 */
	static bool load(const char *path, const char *message);

	// Only for single files, must take care of all the cleanup/registering matrix themselves
	static bool read_file(const char *name);

	static bool finish_loading();

public:
	static void obj_for_xref(obj_type type, const char *name, obj_desc_t *data);
	static void xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal);
	static void resolve_xrefs();
};


#endif
