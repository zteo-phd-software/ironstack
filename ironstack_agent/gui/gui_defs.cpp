#include <stdio.h>
#include <string.h>
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

// color parser
color_t colors::parse(const string& color) {

	// break into tokens
	color_t result = FG_BLACK | BG_BLACK;

	char tokens[2][32];
	sscanf(color.c_str(), "%31s | %31s", tokens[0], tokens[1]);

	for (uint32_t counter = 0; counter < 2; ++counter) {
		if (strcmp(tokens[counter], "FG_BLACK") == 0) {
			result |= FG_BLACK;
		} else if (strcmp(tokens[counter], "FG_BLUE") == 0) {
			result |= FG_BLUE;
		} else if (strcmp(tokens[counter], "FG_GREEN") == 0) {
			result |= FG_GREEN;
		} else if (strcmp(tokens[counter], "FG_CYAN") == 0) {
			result |= FG_CYAN;
		} else if (strcmp(tokens[counter], "FG_RED") == 0) {
			result |= FG_RED;
		} else if (strcmp(tokens[counter], "FG_MAGENTA") == 0) {
			result |= FG_MAGENTA;
		} else if (strcmp(tokens[counter], "FG_BROWN") == 0) {
			result |= FG_BROWN;
		} else if (strcmp(tokens[counter], "FG_WHITE") == 0) {
			result |= FG_WHITE;
		} else if (strcmp(tokens[counter], "FG_GRAY") == 0) {
			result |= FG_GRAY;
		} else if (strcmp(tokens[counter], "FG_LIGHT_BLUE") == 0) {
			result |= FG_LIGHT_BLUE;
		} else if (strcmp(tokens[counter], "FG_LIGHT_GREEN") == 0) {
			result |= FG_LIGHT_GREEN;
		} else if (strcmp(tokens[counter], "FG_LIGHT_CYAN") == 0) {
			result |= FG_LIGHT_CYAN;
		} else if (strcmp(tokens[counter], "FG_LIGHT_RED") == 0) {
			result |= FG_LIGHT_RED;
		} else if (strcmp(tokens[counter], "FG_LIGHT_MAGENTA") == 0) {
			result |= FG_LIGHT_MAGENTA;
		} else if (strcmp(tokens[counter], "FG_YELLOW") == 0) {
			result |= FG_YELLOW;
		} else if (strcmp(tokens[counter], "FG_BRIGHT_WHITE") == 0) {
			result |= FG_BRIGHT_WHITE;
		} else if (strcmp(tokens[counter], "BG_BLACK") == 0) {
			result |= BG_BLACK;
		} else if (strcmp(tokens[counter], "BG_BLUE") == 0) {
			result |= BG_BLUE;
		} else if (strcmp(tokens[counter], "BG_GREEN") == 0) {
			result |= BG_GREEN;
		} else if (strcmp(tokens[counter], "BG_CYAN") == 0) {
			result |= BG_CYAN;
		} else if (strcmp(tokens[counter], "BG_RED") == 0) {
			result |= BG_RED;
		} else if (strcmp(tokens[counter], "BG_MAGENTA") == 0) {
			result |= BG_MAGENTA;
		} else if (strcmp(tokens[counter], "BG_BROWN") == 0) {
			result |= BG_BROWN;
		} else if (strcmp(tokens[counter], "BG_WHITE") == 0) {
			result |= BG_WHITE;
		} else if (strcmp(tokens[counter], "BG_GRAY") == 0) {
			result |= BG_GRAY;
		} else if (strcmp(tokens[counter], "BG_LIGHT_BLUE") == 0) {
			result |= BG_LIGHT_BLUE;
		} else if (strcmp(tokens[counter], "BG_LIGHT_GREEN") == 0) {
			result |= BG_LIGHT_GREEN;
		} else if (strcmp(tokens[counter], "BG_LIGHT_CYAN") == 0) {
			result |= BG_LIGHT_CYAN;
		} else if (strcmp(tokens[counter], "BG_LIGHT_RED") == 0) {
			result |= BG_LIGHT_RED;
		} else if (strcmp(tokens[counter], "BG_LIGHT_MAGENTA") == 0) {
			result |= BG_LIGHT_MAGENTA;
		} else if (strcmp(tokens[counter], "BG_YELLOW") == 0) {
			result |= BG_YELLOW;
		} else if (strcmp(tokens[counter], "BG_BRIGHT_WHITE") == 0) {
			result |= BG_BRIGHT_WHITE;
		}
	}

	return result;
}
