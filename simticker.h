/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMTICKER_H
#define SIMTICKER_H

#include "simcolor.h"

// ticker height
#define TICKER_HEIGHT      (LINESPACE+3)
// ticker vertical position from bottom of screen
#define TICKER_YPOS_BOTTOM (15+TICKER_HEIGHT)

class koord;

/**
 * A very simple news ticker.
 * The news are displayed by karte_vollansicht_t
 */
namespace ticker
{
	bool empty();

	/**
	 * Add a message to the message list
	 * @param pos    position of the event
	 * @param color  message color 
	 */
	void add_msg(const char*, koord pos, int color = COL_BLACK);

	/**
	 * Ticker infowin pops up
	 */
	koord get_welt_pos();

	/**
	 * Ticker redraw
	 */
	void draw();

	/**
	 * Set true if ticker has to be redrawn
	 */
	void set_redraw_all(const bool);

	/**
	 * Ticker text redraw after resize
	 */
	void redraw_ticker();

	void clear_ticker();
};

#endif
