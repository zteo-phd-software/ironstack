#include <assert.h>
#include <memory.h>
#include "gui_component.h"
#include "gui_controller.h"
using namespace gui;

// statically defined variables here
atomic_int gui_component::last_z_depth{10000};
atomic_int input_component::last_input_sequence{10000};

// gui metadata object
gui_metadata::gui_metadata() {
	visible = true;
	z_position = 0;
	alignment = alignment_t::TOP_LEFT;
}

// gui draw state object
gui_draw_state::gui_draw_state() {
	current_color = FG_WHITE | BG_BLACK;
	cursor_color = FG_BLACK | BG_GREEN;
	cursor_visible = false;
}

// constructor
gui_component::gui_component() {
	pending_pad = nullptr;
	active_pad = nullptr;
	pending_state.z_position = get_next_z_depth();
	active_state.z_position = pending_state.z_position;
	needs_redraw = false;
}

// destructor
gui_component::~gui_component() {
	if (pending_pad != nullptr) {
		delwin(pending_pad);
	}
	if (active_pad != nullptr) {
		delwin(active_pad);
	}
	pending_pad = nullptr;
	active_pad = nullptr;
}

// the universal lower stage file parser. at the end of the parse phase,
// pass the kv store on to the (virtual) next stage parser
bool gui_component::load(const string& filename) {

	char line_buf[1024];
	char key[1024] = {0};
	char value[1024] = {0};
	int line_len;
	map<string, string> result;

	FILE* fp = fopen(filename.c_str(), "r");
	while (fp != nullptr && !feof(fp)) {
		memset(line_buf, 0, sizeof(line_buf));
		memset(key, 0, sizeof(key));
		memset(value, 0, sizeof(value));

		if (fgets(line_buf, sizeof(line_buf), fp) == nullptr) break;

		line_len = strlen(line_buf);
		if (line_buf[line_len-1] == '\n') line_buf[line_len-1] = 0;

		// parse each line
		bool storing_key = true;
		int write_offset = 0;
		for (int counter = 0; counter < line_len; ++counter) {
			char current_char = line_buf[counter];
			
			// disregard comments (ignore rest of line when # is encountered)
			// TODO -- allow escaping of comments
			if (current_char == '#') {
				break;

			// ignore whitespaces when parsing key/value pairs
			} else if (current_char == ' ' || current_char == '\t' || current_char == 0) {
				continue;

			// if = is encountered, switch from reading keys to reading values
			} else if (current_char == '=') {
				if (!storing_key) {	// can't have more than one =
					goto failed;
				} else {
					storing_key = false;
					write_offset = 0;
				}

			// otherwise, store the char into the key or the value buffer
			} else {
				if (storing_key) {
					key[write_offset++] = current_char;
				} else {
					value[write_offset++] = current_char;
				}
			}
		}

		// finished parsing the line
		if (strlen(key) > 0 && strlen(value) > 0) {
			string key_string = key;
			string value_string = value;

			result[key] = value;
		}
	}

	// finished parsing the file into key/value pairs
	// transfer control to virtual upper stage parser
	return load_parser(result);

failed:
	fclose(fp);
	return false;
}

// saves the component settings into a file
bool gui_component::save(const string& filename) {
	
	return true;
}

// sets the controller for the gui component
void gui_component::set_controller(const shared_ptr<gui_controller>& controller) {
	output_controller = controller;
}

// gets a pointer to the controller
shared_ptr<gui_controller> gui_component::get_controller() const {
	return output_controller.lock();
}

// sets visibility of gui component
void gui_component::set_visible(bool visible) {
	lock_guard<mutex> g(pending_lock);
	set_visible_unsafe(visible);
}

// set visibility of gui component. thread unsafe version
void gui_component::set_visible_unsafe(bool visible) {
	if (pending_state.visible != visible) {
		pending_state.visible = visible;
		needs_redraw = true;
	}
}

// sets the anchor origin of the gui element
void gui_component::set_origin(const vec2d& origin) {
	lock_guard<mutex> g(pending_lock);
	set_origin_unsafe(origin);
}

// sets anchor origin of gui component. thread unsafe version
void gui_component::set_origin_unsafe(const vec2d& origin) {
	if (pending_state.origin != origin) {
		pending_state.origin = origin;
		needs_redraw = true;
	}
}

// sets the depth of the gui element (negative z = nearer to viewer)
void gui_component::set_z_position(int z) {
	lock_guard<mutex> g(pending_lock);
	set_z_position_unsafe(z);
}

// sets the depth of the gui element. thread unsafe version
void gui_component::set_z_position_unsafe(int z) {
	if (pending_state.z_position != z) {
		pending_state.z_position = z;
		needs_redraw = true;
	}
}

// sets the anchor alignment of the gui element
void gui_component::set_alignment(const alignment_t& alignment) {
	lock_guard<mutex> g(pending_lock);
	set_alignment_unsafe(alignment);
}

// sets the anchor alignment of the gui element. thread unsafe version
void gui_component::set_alignment_unsafe(const alignment_t& alignment) {
	if (pending_state.alignment != alignment) {
		pending_state.alignment = alignment;
		needs_redraw = true;
	}
}

// sets the dimensions of the gui element
void gui_component::set_dimensions(const vec2d& dimensions) {
	lock_guard<mutex> g(pending_lock);
	set_dimensions_unsafe(dimensions);
}

// sets the dimensions of the gui element. thread unsafe version
void gui_component::set_dimensions_unsafe(const vec2d& dimensions) {
	if (pending_state.dimensions != dimensions) {
		pending_state.dimensions = dimensions;

		// remove old pad if any
		if (pending_pad != nullptr) {
			delwin(pending_pad);
		}

		// create new pad
		pending_pad = newpad(dimensions.y, dimensions.x);
		if (pending_pad == nullptr) {
			return;
		}

		// clear pad to current color
		clear_pad_unsafe();
		needs_redraw = true;
	}
}

// sets the current output color
void gui_component::set_color(const color_t& color) {
	lock_guard<mutex> g(pending_lock);
	set_color_unsafe(color);
}

// sets the current output color. thread unsafe version
void gui_component::set_color_unsafe(const color_t& color) {
	pending_draw_state.current_color = color;
}

// sets the cursor location
void gui_component::set_cursor(const vec2d& location) {
	lock_guard<mutex> g(pending_lock);
	set_cursor_unsafe(location);
}

// sets the cursor location. thread unsafe version
void gui_component::set_cursor_unsafe(const vec2d& location) {

	// clamp cursor to within valid rectangle
	vec2d new_location = location;
	if (new_location.x < 0) {
		new_location.x = 0;
	} else if (new_location.x >= pending_state.dimensions.x) {
		new_location.x = pending_state.dimensions.x-1;
	}
	if (new_location.y < 0) {
		new_location.y = 0;
	} else if (new_location.y >= pending_state.dimensions.y) {
		new_location.y = pending_state.dimensions.y-1;
	}

	// update cursor position as necessary
	if (new_location != pending_draw_state.cursor_position) {
		pending_draw_state.cursor_position = new_location;
		needs_redraw = true;
	}
}

// sets the cursor color
void gui_component::set_cursor_color(const color_t& color) {
	lock_guard<mutex> g(pending_lock);
	set_cursor_color_unsafe(color);
}

// sets the cursor color. thread unsafe version
void gui_component::set_cursor_color_unsafe(const color_t& color) {
	pending_draw_state.cursor_color = color;
}

// makes the cursor visible if required
void gui_component::set_cursor_visible(bool visible) {
	lock_guard<mutex> g(pending_lock);
	set_cursor_visible_unsafe(visible);
}

// changes visibility of the cursor. thread unsafe version
void gui_component::set_cursor_visible_unsafe(bool visible) {
	if (pending_draw_state.cursor_visible != visible) {
		pending_draw_state.cursor_visible = visible;
		needs_redraw = true;
	}
}

// provides the default implementation for clearing the gui component
void gui_component::clear() {
	lock_guard<mutex> g(pending_lock);
	if (pending_pad != nullptr) {
		clear_pad_unsafe();
		needs_redraw = true;
	}
}

// provides the default implementation for printing to the gui component
void gui_component::printf(const char* fmt, ...) {
	lock_guard<mutex> g(pending_lock);

	va_list args;
	va_start(args, fmt);
	vprintf_unsafe(fmt, args);
	va_end(args);
}

// provides the thread unsafe version of printing onto the gui component
void gui_component::printf_unsafe(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf_unsafe(fmt, args);
	va_end(args);
}

// default printf. thread unsafe version
void gui_component::vprintf_unsafe(const char* fmt, va_list args) {
	if (pending_pad == nullptr) return;

	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);
	change_color_unsafe(pending_pad, pending_draw_state.current_color);
	multiline_print_unsafe(buf);
	needs_redraw = true;
}

// provides the default implementation for printing to a given location on the gui component
void gui_component::printf(const vec2d& location, const char* fmt, ...) {
	lock_guard<mutex> g(pending_lock);
	va_list args;
	va_start(args, fmt);
	vprintf_unsafe(location, fmt, args);
	va_end(args);
}

// prints to a given location on the screen. thread unsafe
void gui_component::printf_unsafe(const vec2d& location, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf_unsafe(location, fmt, args);
	va_end(args);
}

// printf at a given location. thread unsafe version
void gui_component::vprintf_unsafe(const vec2d& location, const char* fmt, va_list args) {
	if (pending_pad == nullptr) return;

	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);

	set_cursor_unsafe(location);
	change_color_unsafe(pending_pad, pending_draw_state.current_color);
	multiline_print_unsafe(buf);
	needs_redraw = true;
}

// same as printf but temporarily changes the color
void gui_component::printfc(color_t color, const char* fmt, ...) {
	lock_guard<mutex> g(pending_lock);
	va_list args;
	va_start(args, fmt);
	vprintfc_unsafe(color, fmt, args);
	va_end(args);
}

// same as printf but temporarilt changes the color. thread unsafe.
void gui_component::printfc_unsafe(color_t color, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintfc_unsafe(color, fmt, args);
	va_end(args);
}

// printf with temporary color change. thread unsafe version
void gui_component::vprintfc_unsafe(color_t color, const char* fmt, va_list args) {
	if (pending_pad == nullptr) return;

	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);

	change_color_unsafe(pending_pad, color);
	multiline_print_unsafe(buf);
	change_color_unsafe(pending_pad, pending_draw_state.current_color);
	needs_redraw = true;
}

// same as printf but sets print location and temporarily changes color
void gui_component::printfc(color_t color, const vec2d& location, const char* fmt, ...) {
	lock_guard<mutex> g(pending_lock);
	va_list args;
	va_start(args, fmt);
	vprintfc_unsafe(color, location, fmt, args);
	va_end(args);
}

// same as printf but sets print location and temporarily changes color. thread unsafe.
void gui_component::printfc_unsafe(color_t color, const vec2d& location, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintfc_unsafe(color, location, fmt, args);
	va_end(args);
}

// printf with temporary color change. thread unsafe version
void gui_component::vprintfc_unsafe(color_t color, const vec2d& location, const char* fmt, va_list args) {
	if (pending_pad == nullptr) return;

	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, args);

	set_cursor_unsafe(location);
	change_color_unsafe(pending_pad, color);
	multiline_print_unsafe(buf);
	change_color_unsafe(pending_pad, pending_draw_state.current_color);
	needs_redraw = true;
}

// finalizes changes to the object metadata
void gui_component::commit_attributes() {
	lock_guard<mutex> g(pending_lock);
	lock_guard<mutex> g2(active_lock);
	commit_attributes_unsafe();
}

// finalizes changes to the object metadata, thread unsafe version
void gui_component::commit_attributes_unsafe() {
	
	// treat the case of changing dimensions separately because the front buffer may
	// be a different size than the back buffer and the front buffer should retain
	// its size until a commit_output() is done.
	vec2d old_dimensions = active_state.dimensions;
	active_state = pending_state;
	active_state.dimensions = old_dimensions;
}

// finalizes changes to the object output
void gui_component::commit_output() {
	lock_guard<mutex> g(pending_lock);
	lock_guard<mutex> g2(active_lock);
	commit_output_unsafe();
}

// stages the gui component for display later. default implementation copies from
// pad to window and displays cursor (if needed)
void gui_component::commit_output_unsafe() {

	// if dimensions have changed, realloc new active pad
	if (pending_state.dimensions != active_state.dimensions) {
		if (active_pad != nullptr) {
			delwin(active_pad);
		}
		active_pad = newpad(pending_state.dimensions.y, pending_state.dimensions.x);
		if (active_pad == nullptr) {
			return;
		}
		active_state.dimensions = pending_state.dimensions;
	}

	// update contents from pending to active pad
	if (copywin(pending_pad, active_pad,
		0,                    // sminrow
		0,                    // smincol
		0,                    // dminrow
		0,                    // dmincol
		pending_state.dimensions.y-1, // dmaxrow
		pending_state.dimensions.x-1, // dmaxcol
		FALSE) == ERR) {			// false = no overlay
		
		assert(false && "unable to perform copywin from pending to active pad.");
		abort();
	}

	// if there is a cursor, display it
	if (pending_draw_state.cursor_visible) {
		char buf[2];
		mvwinnstr(active_pad, pending_draw_state.cursor_position.y, pending_draw_state.cursor_position.x, buf, 1);
		change_color_unsafe(active_pad, pending_draw_state.cursor_color);
		mvwaddch(active_pad, pending_draw_state.cursor_position.y, pending_draw_state.cursor_position.x, buf[0]);
		change_color_unsafe(active_pad, pending_draw_state.current_color);
	}

	// update draw state
	active_draw_state = pending_draw_state;
}

// finalizes both metadata and output changes
void gui_component::commit_all() {
	lock_guard<mutex> g(pending_lock);
	lock_guard<mutex> g2(active_lock);
	commit_attributes_unsafe();
	commit_output_unsafe();
}

// called by the controller to perform lazy render. default implementation
// does nothing
void gui_component::render() {}

// checks if the gui component is visible
bool gui_component::is_visible() const {
	lock_guard<mutex> g(active_lock);
	return active_state.visible;
}

// returns the anchor origin of the gui component
vec2d gui_component::get_origin() const {
	lock_guard<mutex> g(active_lock);
	return active_state.origin;
}

// returns the z depth of the gui component
int gui_component::get_z_position() const {
	lock_guard<mutex> g(active_lock);
	return active_state.z_position;
}

// returns the dimensions of the gui component
vec2d gui_component::get_dimensions() const {
	lock_guard<mutex> g(active_lock);
	return active_state.dimensions;
}

// returns the current draw color
color_t gui_component::get_color() const {
	lock_guard<mutex> g(active_lock);
	return active_draw_state.current_color;
}

// returns position of the cursor
vec2d gui_component::get_cursor() const {
	lock_guard<mutex> g(active_lock);
	return active_draw_state.cursor_position;
}

// returns the color of the cursor
color_t gui_component::get_cursor_color() const {
	lock_guard<mutex> g(active_lock);
	return active_draw_state.cursor_color;
}

// checks if the cursor is visible
bool gui_component::is_cursor_visible() const {
	lock_guard<mutex> g(active_lock);
	return active_draw_state.cursor_visible;
}

// static function. gets the next available z depth
int gui_component::get_next_z_depth() {
	return last_z_depth++;
}

// private helper function. sets the color attribute on the screen
void gui_component::change_color_unsafe(WINDOW* target, uint8_t color) {

	uint8_t base_foreground_color = color & 7;
	bool foreground_color_bright = ((color & 8) > 0);
	uint8_t base_background_color = (color & (7 << 4)) >> 4;	
	uint8_t translated_color_index = base_foreground_color + (base_background_color << 3);

	wattrset(target, COLOR_PAIR(translated_color_index) | (foreground_color_bright ? A_BOLD : 0));
}

// private helper function. clears the pad and reset cursor position
void gui_component::clear_pad_unsafe() {
	change_color_unsafe(pending_pad, pending_draw_state.current_color);
	for (int counter = 0; counter < pending_state.dimensions.y; ++counter) {
		for (int counter2 = 0; counter2 < pending_state.dimensions.x; ++counter2) {
			mvwaddch(pending_pad, counter, counter2, ' ');
		}
	}
	wmove(pending_pad, 0, 0);
	pending_draw_state.cursor_position.x = 0;
	pending_draw_state.cursor_position.y = 0;
	needs_redraw = true;
}

// performs a potentially multiline print on the gui component. updates cursor
void gui_component::multiline_print_unsafe(const char* text) {

	int line_width = pending_state.dimensions.x;
	int text_len = strlen(text);
	int current_text_offset = 0;
	int& current_cursor_x = pending_draw_state.cursor_position.x;
	int& current_cursor_y = pending_draw_state.cursor_position.y;
	char current_char;
	
	while(current_text_offset < text_len) {
		current_char = text[current_text_offset];

		// handle backspace
		if (current_char == '\b') {
			--current_cursor_x;
			if (current_cursor_x < 0) {
				--current_cursor_y;
				if (current_cursor_y < 0) {
					current_cursor_x = 0;
					current_cursor_y = 0;
				} else {
					current_cursor_x = line_width-1;
				}
			}

		// handle tabs
		} else if (current_char == '\t') {
			if (current_cursor_x + 8 >= line_width) {
				current_cursor_x = line_width-1;
				pad_to_eol_unsafe();
			} else {
				mvwaddstr(pending_pad, current_cursor_y, current_cursor_x, "        ");
				current_cursor_x += 8;
			}

		// handle newlines
		} else if (current_char == '\r' || current_char == '\n') {
			current_cursor_x = 0;
			++current_cursor_y;
			if (current_cursor_y >= pending_state.dimensions.y) {
				current_cursor_y = pending_state.dimensions.y-1;
				scroll_unsafe();
			}

		// handle normal characters
		} else if (current_char >= 32 && current_char <= 126) {
			if (current_cursor_y >= pending_state.dimensions.y) {
				current_cursor_y = pending_state.dimensions.y-1;
			}
			mvwaddch(pending_pad, current_cursor_y, current_cursor_x, current_char);
			++current_cursor_x;

			if (current_cursor_x >= line_width) {
				current_cursor_x = 0;
				++current_cursor_y;
				if (current_cursor_y >= pending_state.dimensions.y) {
					current_cursor_y = pending_state.dimensions.y-1;
					scroll_unsafe();
				}
			} 
		}

		// increment text offset
		++current_text_offset;
	}
}	

// helper function. writes spaces till the end of the line. does not update
// cursor position
void gui_component::pad_to_eol_unsafe() {
	int spaces_needed = pending_state.dimensions.x-pending_draw_state.cursor_position.x;
	if (spaces_needed <= 0) return;

	char spacer[spaces_needed+1];
	memset(spacer, ' ', spaces_needed);
	spacer[spaces_needed] = 0;

	mvwaddstr(pending_pad, pending_draw_state.cursor_position.y, pending_draw_state.cursor_position.x, spacer);
}

// helper function. scrolls contents of the pad up by one
void gui_component::scroll_unsafe() {
	copywin(pending_pad, pending_pad, 1, 0, 0, 0, pending_state.dimensions.y-2, pending_state.dimensions.x-1, FALSE);
	char buf[pending_state.dimensions.x+1];
	memset(buf, ' ', pending_state.dimensions.x);
	buf[pending_state.dimensions.x+1] = 0;
	mvwaddstr(pending_pad, pending_state.dimensions.y-1, 0, buf);
}

// used to acquire the lock before performing gui component load
bool gui_component::load_parser(const map<string, string>& kv_store) {
	lock_guard<mutex> g(pending_lock);
	return load_parser_unsafe(kv_store);
}

// actual function that does the loading work
bool gui_component::load_parser_unsafe(const map<string, string>& kv_store) {

	map<string, string>::const_iterator iterator;

	// parse the visibility tag
	iterator = kv_store.find("visible");
	if (iterator != kv_store.end()) {
		if (iterator->second == "yes" || iterator->second == "true") {
			set_visible_unsafe(true);
		} else if (iterator->second == "no" || iterator->second == "false") {
			set_visible_unsafe(false);
		}
	}

	// parse origin tag
	iterator = kv_store.find("origin");
	if (iterator != kv_store.end()) {
		vec2d location;
		if (location.parse(iterator->second)) {
			set_origin_unsafe(location);
		}
	}

	// parse z position
	iterator = kv_store.find("z_position");
	if (iterator != kv_store.end()) {
		int depth = 0;
		if (sscanf(iterator->second.c_str(), "%d", &depth) == 1) {
			set_z_position_unsafe(depth);
		}
	}

	// parse alignment
	iterator = kv_store.find("alignment");
	if (iterator != kv_store.end()) {
		const string& result = iterator->second;
		if (result == "default" || result == "top_left") {
			set_alignment_unsafe(alignment_t::TOP_LEFT);
		} else if (result == "top_center") {
			set_alignment_unsafe(alignment_t::TOP_CENTER);
		} else if (result == "top_right") {
			set_alignment_unsafe(alignment_t::TOP_RIGHT);
		} else if (result == "middle_left") {
			set_alignment_unsafe(alignment_t::MIDDLE_LEFT);
		} else if (result == "middle_center") {
			set_alignment_unsafe(alignment_t::MIDDLE_CENTER);
		} else if (result == "middle_right") {
			set_alignment_unsafe(alignment_t::MIDDLE_RIGHT);
		} else if (result == "bottom_left") {
			set_alignment_unsafe(alignment_t::BOTTOM_LEFT);
		} else if (result == "bottom_center") {
			set_alignment_unsafe(alignment_t::BOTTOM_CENTER);
		} else if (result == "bottom_right") {
			set_alignment_unsafe(alignment_t::BOTTOM_RIGHT);
		}
	}

	// parse dimensions
	iterator = kv_store.find("dimensions");
	if (iterator != kv_store.end()) {
		vec2d dimensions;
		if (dimensions.parse(iterator->second)) {
			set_dimensions_unsafe(dimensions);
		}
	}

	return true;
}

// used to generate key/value pairs for later commit onto disk
map<string, string> gui_component::save_parser() const {

	map<string, string> result;
	return result;
}

// constructor for input component sets input sequence
input_component::input_component() {
	enabled = true;
	input_sequence = get_next_input_sequence();
}

// destructor for input component unregisters the object if required
input_component::~input_component() {
	shared_ptr<gui_controller> controller = input_controller.lock();
	if (controller != nullptr) {
		controller->unregister_input_component(shared_from_this());
	}
}

// sets the controller
void input_component::set_controller(const shared_ptr<gui_controller>& controller) {
	input_controller = controller;
}

// enable or disable input processing on a component
void input_component::set_enable(bool enabled_) {
	enabled = enabled_;

	shared_ptr<gui_controller> controller = input_controller.lock();
	if (controller != nullptr) {
		controller->set_enable(shared_from_this(), enabled);
	}
}

// check if the component is enabled or disabled
bool input_component::is_enabled() const {
	return enabled;
}

// sets the sequence for the input component
void input_component::set_input_sequence(int sequence) {
	input_sequence = sequence;
}

// retrieves the input sequence
int input_component::get_input_sequence() const {
	return input_sequence;
}

// tells the controller to give this object input focus
void input_component::set_focus() {
	shared_ptr<gui_controller> controller = input_controller.lock();
	if (controller != nullptr) {
		controller->set_focus(shared_from_this());
	}
}

// tells the controller to give up focus on this object, thus allowing another
// object the input focus
void input_component::relinquish_focus() {
	shared_ptr<gui_controller> controller = input_controller.lock();
	if (controller != nullptr) {
		controller->relinquish_focus(shared_from_this());
	}
}

// provides the default implementation for some callbacks
void input_component::on_register() {}
void input_component::on_unregister() {}
void input_component::on_focus() {}
void input_component::on_blur() {}

// static function. returns the next available input sequence
int input_component::get_next_input_sequence() {
	return last_input_sequence++;
}

