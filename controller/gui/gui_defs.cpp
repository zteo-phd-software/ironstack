#include <stdio.h>
#include "gui_defs.h"
using namespace gui;

// clears a 2d vector
void vec2d::clear() {
	x = 0;
	y = 0;
}

// parses a 2d vector of the form (x,y)
// note: does not do sanity checking beyond the closing parenthesis.
bool vec2d::parse(const string& loc) {

	int x_, y_;
	if (sscanf(loc.c_str(), "(%d,%d)", &x_, &y_) != 2) {
		x = 0;
		y = 0;
		return false;
	} else {
		x = x_;
		y = y_;
		return true;
	}

}
