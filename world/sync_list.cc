
#include "sync_list.h"

#include "../ifc/sync_steppable.h"


sync_list_t::sync_list_t() :
	currently_deleting(NULL),
	sync_step_running(false)
{
}


sync_list_t::~sync_list_t()
{
}


void sync_list_t::add(sync_steppable *obj)
{
	assert(!sync_step_running);
	list.append(obj);
}


void sync_list_t::remove(sync_steppable *obj)
{
	if(sync_step_running) {
		assert(obj != currently_deleting);
	}
	else {
		list.remove(obj);
	}
}


void sync_list_t::clear()
{
	list.clear();
	currently_deleting = NULL;
	sync_step_running = false;
}


void sync_list_t::sync_step(uint32 delta_t)
{
	sync_step_running = true;
	currently_deleting = NULL;

	for(  uint32 i=0;  i<list.get_count();  i++  ) {
		sync_steppable *ss = list[i];

		switch(ss->sync_step(delta_t)) {
		case SYNC_OK:
			break;

		case SYNC_DELETE:
			currently_deleting = ss;
			delete ss;
			currently_deleting = NULL;
			/* fall-through */

		case SYNC_REMOVE:
			ss = list.pop_back();
			if (i < list.get_count()) {
				list[i] = ss;
			}
		}
	}

	sync_step_running = false;
}

