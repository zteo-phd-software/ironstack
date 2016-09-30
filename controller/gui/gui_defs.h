#pragma once
#include <string>
using namespace std;

// defines gui component definitions
namespace gui {

	// defines a vector type
	class vec2d {
	public:
		vec2d():x(0),y(0) {}
		vec2d(int x_, int y_):x(x_),y(y_) {}
		void clear();
		void set(int x_, int y_) { x=x_; y=y_; }
		bool parse(const string& loc);

		bool operator==(const vec2d& other) const { return x==other.x && y==other.y; }
		bool operator!=(const vec2d& other) const { return !(*this==other); }

		int x;
		int y;
	};

	// define some alignment types
	enum class alignment_t {
		TOP_LEFT, TOP_CENTER, TOP_RIGHT,
		MIDDLE_LEFT, MIDDLE_CENTER, MIDDLE_RIGHT,
		BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT
	};

  // key definitions come here
	enum class special_key_t : uint8_t {
		NONE = 255,
		ARROW_DOWN = 40,
		ARROW_UP = 38,
		ARROW_LEFT = 37,
		ARROW_RIGHT = 39,
		HOME = 36,
		PAGE_DOWN = 34,
		PAGE_UP = 33,
		END = 35,
		DELETE = 46
	};


	// defines keystrokes
	class keystroke {
	public:
		// constructors
		keystroke():ascii_value(0),
			special_key(special_key_t::NONE) {}
		keystroke(uint8_t asc_value, special_key_t spec):
			ascii_value(asc_value),
			special_key(spec) {}

		// clear the object
		void clear() { ascii_value = 0; special_key = special_key_t::NONE; }

		// used for key comparisons
		bool operator<(const keystroke& other) const { 
			if (ascii_value == other.ascii_value) {
				return (uint8_t) special_key < (uint8_t) other.special_key;
			} else {
				return ascii_value < other.ascii_value;
			}
		}

		// check if this is a valid key
		bool is_valid() const { return (ascii_value != 0 || special_key != special_key_t::NONE); }

		// publicly accessible keys
		uint8_t       ascii_value;
		special_key_t	special_key;
	};

	typedef uint8_t color_t;

	static const uint8_t FG_BLACK = 0;
	static const uint8_t FG_BLUE = 1;
	static const uint8_t FG_GREEN = 2;
	static const uint8_t FG_CYAN = 3;
	static const uint8_t FG_RED = 4;
	static const uint8_t FG_MAGENTA = 5;
	static const uint8_t FG_BROWN = 6;
	static const uint8_t FG_WHITE = 7;
	static const uint8_t FG_GRAY = 8;
	static const uint8_t FG_LIGHT_BLUE = 9;
	static const uint8_t FG_LIGHT_GREEN = 10;
	static const uint8_t FG_LIGHT_CYAN = 11;
	static const uint8_t FG_LIGHT_RED = 12;
	static const uint8_t FG_LIGHT_MAGENTA = 13;
	static const uint8_t FG_YELLOW = 14;
	static const uint8_t FG_BRIGHT_WHITE = 15;

	static const uint8_t BG_BLACK = 0;
	static const uint8_t BG_BLUE = (1 << 4);
	static const uint8_t BG_GREEN = (2 << 4);
	static const uint8_t BG_CYAN = (3 << 4);
	static const uint8_t BG_RED = (4 << 4);
	static const uint8_t BG_MAGENTA = (5 << 4);
	static const uint8_t BG_BROWN = (6 << 4);
	static const uint8_t BG_WHITE = (7 << 4);
	static const uint8_t BG_GRAY = (8 << 4);
	static const uint8_t BG_LIGHT_BLUE = (9 << 4);
	static const uint8_t BG_LIGHT_GREEN = (10 << 4);
	static const uint8_t BG_LIGHT_CYAN = (11 << 4);
	static const uint8_t BG_LIGHT_RED = (12 << 4);
	static const uint8_t BG_LIGHT_MAGENTA = (13 << 4);
	static const uint8_t BG_YELLOW = (14 << 4);
	static const uint8_t BG_BRIGHT_WHITE = (15 << 4);

};
