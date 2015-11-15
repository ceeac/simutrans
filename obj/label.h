#ifndef label_h
#define label_h

#include "../simobj.h"
#include "../display/simimg.h"

class label_t : public obj_t
{
public:
	label_t(loadsave_t *file);
	label_t(koord3d pos, player_t *player, const char *text);
	~label_t();

	void finish_rd();

	void show_info();

	typ get_typ() const { return obj_t::label; }

	image_id get_image() const;
};

#endif
