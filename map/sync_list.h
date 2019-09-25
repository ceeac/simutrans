
#ifndef sync_list_h
#define sync_list_h


#include "../tpl/vector_tpl.h"


class sync_steppable;


/**
 * List of synchronous objects, for game update (sync_step)
 */
class sync_list_t
{
public:
	sync_list_t();
	~sync_list_t();

public:
	/// Append an object to this list. Must not be called during sync_step.
	void add(sync_steppable *obj);

	/// Removes an object from this list, but does not delete it.
	/// Does nothing when called during a sync_step.
	void remove(sync_steppable *obj);

	/// do a sync_step for all objects in this list, and deleting objects if necessary.
	void sync_step(uint32 delta_t);

	/// clear list, do not delete the objects
	void clear();

private:
	vector_tpl<sync_steppable *> list;  ///< list of sync-steppable objects
	sync_steppable* currently_deleting; ///< deleted during sync_step, safeguard calls to remove
	bool sync_step_running;
};

#endif
