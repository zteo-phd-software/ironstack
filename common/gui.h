#ifndef _TEXT_GUI
#define _TEXT_GUI

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <ncurses.h>
#include <unistd.h>
#endif

#include "common_utils.h"
#include <list>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct
{
	uint8_t character;
	uint8_t composite_color;

} output_char_t;

// define color mappings here
#define COLOR_FOREGROUND_BLACK 0
#define COLOR_FOREGROUND_BLUE 1
#define COLOR_FOREGROUND_GREEN 2
#define COLOR_FOREGROUND_CYAN 3
#define COLOR_FOREGROUND_RED 4
#define COLOR_FOREGROUND_MAGENTA 5
#define COLOR_FOREGROUND_BROWN 6
#define COLOR_FOREGROUND_WHITE 7
#define COLOR_FOREGROUND_GRAY 8
#define COLOR_FOREGROUND_LIGHT_BLUE 9
#define COLOR_FOREGROUND_LIGHT_GREEN 10
#define COLOR_FOREGROUND_LIGHT_CYAN 11
#define COLOR_FOREGROUND_LIGHT_RED 12
#define COLOR_FOREGROUND_LIGHT_MAGENTA 13
#define COLOR_FOREGROUND_YELLOW 14
#define COLOR_FOREGROUND_BRIGHT_WHITE 15

#define COLOR_BACKGROUND_BLACK 0
#define COLOR_BACKGROUND_BLUE (1 << 4)
#define COLOR_BACKGROUND_GREEN (2 << 4)
#define COLOR_BACKGROUND_CYAN (3 << 4)
#define COLOR_BACKGROUND_RED (4 << 4)
#define COLOR_BACKGROUND_MAGENTA (5 << 4)
#define COLOR_BACKGROUND_BROWN (6 << 4)
#define COLOR_BACKGROUND_WHITE (7 << 4)
#define COLOR_BACKGROUND_GRAY (8 << 4)
#define COLOR_BACKGROUND_LIGHT_BLUE (9 << 4)
#define COLOR_BACKGROUND_LIGHT_GREEN (10 << 4)
#define COLOR_BACKGROUND_LIGHT_CYAN (11 << 4)
#define COLOR_BACKGROUND_LIGHT_RED (12 << 4)
#define COLOR_BACKGROUND_LIGHT_MAGENTA (13 << 4)
#define COLOR_BACKGROUND_YELLOW (14 << 4)
#define COLOR_BACKGROUND_BRIGHT_WHITE (15 << 4)

// define keys here
#define KEYBOARD_KEY_DOWN 40
#define KEYBOARD_KEY_UP 38
#define KEYBOARD_KEY_LEFT 37
#define KEYBOARD_KEY_RIGHT 39
#define KEYBOARD_KEY_HOME 36
#define KEYBOARD_KEY_NPAGE 34
#define KEYBOARD_KEY_PPAGE 33
#define KEYBOARD_KEY_END 35

// screen refresh intervals
#define DEFAULT_REFRESH_INTERVAL 0.5

// default window definitions
#define DEFAULT_ROWS 25
#define DEFAULT_COLUMNS 80

// text panel definitions
class text_panel
{
#ifndef NO_GUI

public:

	text_panel();
	virtual ~text_panel();

	// indirect modifiers
	void set_panel_origin(int32_t new_x_origin, int32_t new_y_origin);
	void set_panel_dimensions(uint32_t new_x_width, uint32_t new_y_height);
	void set_z_position(int32_t new_z_depth);
	void set_visibility(bool new_visibility_state);
	void register_panel();
	void unregister_panel();

	// direct modifiers
	virtual void write(const void* message, ...)=0;	// only this function has to be overloaded in derivative classes
	virtual void write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...);
	void write_no_commit(const void* message, ...);
	void write_pos_no_commit(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...);
	inline void write_commit() { commit_render_buffer(); }
	void move_cursor_absolute(uint32_t rel_x_position, uint32_t rel_y_position);	// absolute wrt to window coordinates
	void move_cursor_relative(int32_t x_displacement, int32_t y_displacement);		// relative wrt to window coordinates

	void clear();
	void clear_no_commit();
	inline void set_color(uint8_t composite_color) { current_color = composite_color; }

	// object accessors
	inline void get_absolute_cursor_coordinates(uint32_t* abs_x_position, uint32_t* abs_y_position) { *abs_x_position = current_cursor_column + abs_x_origin; *abs_y_position = current_cursor_row + abs_y_origin; }
	inline void get_relative_cursor_coordinates(uint32_t* rel_x_position, uint32_t* rel_y_position) { *rel_x_position = current_cursor_column, *rel_y_position = current_cursor_row; }
	inline void get_panel_origin(int32_t* x_origin, int32_t* y_origin) { *x_origin = abs_x_origin; *y_origin = abs_y_origin; }
	inline void get_panel_dimensions(uint32_t* x_width, uint32_t* y_height) { *x_width = columns; *y_height = rows; }
	inline void get_z_position(int32_t* z_depth) { *z_depth = z_position; }
	inline void get_visibility(bool* visibility) { *visibility = visible; }
	inline uint8_t get_color() const { return current_color; }

	void scroll_render_buffer(uint32_t lines_to_scroll);

	// commits to buffer and calls for refresh
	void repaint();
	
protected:

	void commit_render_buffer();

	int32_t abs_x_origin, abs_y_origin;
	uint32_t rows, columns;
	int32_t z_position;

	bool visible;
	uint32_t current_cursor_row, current_cursor_column;
	uint8_t current_color;

	// panel data
	bool commit_flag;
	output_char_t* render_buffer;

	//pthread_mutex_t internal_lock;

	pthread_mutex_t intermediate_buffer_lock;
	output_char_t* intermediate_buffer;

	pthread_mutex_t output_buffer_lock;
	output_char_t* output_buffer;

	static const uint32_t MESSAGE_BUFFER_SIZE = 1024;

	friend class display_controller;

private:
	// copy constructor and assignment operator disabled
	text_panel(const text_panel& original);
	text_panel& operator= (const text_panel& original);

#else

public:

	text_panel() {}
	~text_panel() {}

	// indirect modifiers
	inline void set_panel_origin(int32_t new_x_origin, int32_t new_y_origin) {}
	inline void set_panel_dimensions(uint32_t new_x_width, uint32_t new_y_height) {}
	inline void set_z_position(int32_t new_z_depth) {}
	inline void set_visibility(bool new_visibility_state) {}
	inline void register_panel() {}
	inline void unregister_panel() {}

	// direct modifiers
	virtual void write(const void* message, ...)=0;	// only this function has to be overloaded in derivative classes
	inline virtual void write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...) {}
	inline void write_no_commit(const void* message, ...) {}
	inline void write_pos_no_commit(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...) {}
	inline void write_commit() {}
	inline void move_cursor_absolute(uint32_t rel_x_position, uint32_t rel_y_position) {}	// absolute wrt to window coordinates
	inline void move_cursor_relative(int32_t x_displacement, int32_t y_displacement) {}		// relative wrt to window coordinates

	inline void clear() {}
	inline void clear_no_commit() {}
	inline void set_color(uint8_t composite_color) {}

	// object accessors
	inline void get_absolute_cursor_coordinates(uint32_t* abs_x_position, uint32_t* abs_y_position) {}
	inline void get_relative_cursor_coordinates(uint32_t* rel_x_position, uint32_t* rel_y_position) {}
	inline void get_panel_origin(int32_t* x_origin, int32_t* y_origin) {}
	inline void get_panel_dimensions(uint32_t* x_width, uint32_t* y_height) {}
	inline void get_z_position(int32_t* z_depth) {}
	inline void get_visibility(bool* visibility) {}
	inline uint8_t get_color() const { return 0; }

	// commits to buffer and calls for refresh
	inline void repaint() {}
	
private:
	// copy constructor and assignment operator disabled
	text_panel(const text_panel& original);
	text_panel& operator= (const text_panel& original);

#endif
};

// textbox definitions
class textbox_t : public text_panel
{
#ifndef NO_GUI

public:

	textbox_t() {};
	~textbox_t() {};

	virtual void write(const void* message, ...);

private:

	// copy constructor and assignment operator disabled
	textbox_t(const textbox_t& original) {};
	textbox_t& operator=(const textbox_t& original) { return *this; };

#else

public:

	textbox_t() {}
	~textbox_t() {}

	inline virtual void write(const void* message, ...) {}

private:

	// copy constructor and assignment operator disabled
	textbox_t(const textbox_t& original) {}
	textbox_t& operator=(const textbox_t& original) { return *this; }

#endif

};

// input event definition
class input_event_t
{
public:
	
	input_event_t() { keyboard_event = false; special_keyboard_key_activated = false; ascii_value = 0; special_key_value = 0; mouse_clicked = false; abs_x_coordinates = 0; abs_y_coordinates = 0;}

	// function to clear the event type
	inline void clear() { keyboard_event = false; special_keyboard_key_activated = false; ascii_value = 0; special_key_value = 0; mouse_clicked = false; abs_x_coordinates = 0; abs_y_coordinates = 0;}

	// true means keyboard event; false means mouse event
	bool keyboard_event;

	// true means special keyboard key was activated
	bool special_keyboard_key_activated;
	uint8_t ascii_value;
	uint16_t special_key_value;

	// true means mouse clicked; false means mouse move
	bool mouse_clicked;
	uint32_t abs_x_coordinates;
	uint32_t abs_y_coordinates;
};

// input panel definitions
class input_panel : public textbox_t
{
#ifndef NO_GUI

public:

	input_panel();
	~input_panel();

	// overridden methods
	void set_visibility(bool new_visibility_state);

	// turns cursor display on/off during focus event
	inline void set_cursor_on_focus(bool state) { focus_requires_cursor = state; }

	// enable or disable events
	inline void set_keypress_processing(bool state) { accepts_keypress = state; }
	inline void set_mouse_processing(bool state) { accepts_mouse = state; }

	// overriden default text_panel functions, automatically signals display controller
	void register_panel();
	void unregister_panel();

	// sets modal or focus functions
	void set_modal(bool modal_state);
	void set_focus();

protected:

	// mouse events
	virtual void process_click(uint32_t rel_x, uint32_t rel_y)=0;
	virtual void process_mousemove(uint32_t rel_x, uint32_t rel_y)=0;
	virtual void process_mouseout()=0;

	// keyboard events
	virtual void process_keypress(uint8_t ascii_value, uint16_t special_value)=0;

	// refresh events
	virtual void on_focus()=0;
	virtual void off_focus()=0;

private:
	
	// disable copy constructor and assignment operator
	input_panel(const input_panel& original);
	input_panel& operator= (const input_panel& original);

	bool registered;

	bool focus_requires_cursor;

	bool accepts_keypress;
	bool accepts_mouse;

	bool is_modal;
	bool wants_focus;

	friend class display_controller;

#else

public:

	input_panel() {}
	~input_panel() {}

	// overridden methods
	inline void set_visibility(bool new_visibility_state) {}

	// turns cursor display on/off during focus event
	inline void set_cursor_on_focus(bool state) {}

	// enable or disable events
	inline void set_keypress_processing(bool state) {}
	inline void set_mouse_processing(bool state) {}

	// overriden default text_panel functions, automatically signals display controller
	inline void register_panel() {}
	inline void unregister_panel() {}

	// sets modal or focus functions
	inline void set_modal(bool modal_state) {}
	inline void set_focus() {}

private:
	
	// disable copy constructor and assignment operator
	input_panel(const input_panel& original) {}
	input_panel& operator= (const input_panel& original) {}

#endif
};

// enhanced textbox definitions
class enhanced_textbox_t : public textbox_t
{
#ifndef NO_GUI

public:
	
	enhanced_textbox_t();
	~enhanced_textbox_t();

	virtual void write(const void* message, ...);

	uint32_t write_lhs(const void* message, ...);
	void write_rhs(uint32_t lhs_id, const void* message, ...);
	
	void set_divider_position(uint32_t divider_position);
	inline void set_timestamp_display(bool state) { display_timestamp = state; }

private:
	
	// disabled copy constructor and assignment operator
	enhanced_textbox_t(const enhanced_textbox_t& original);
	enhanced_textbox_t& operator=(const enhanced_textbox_t& original);

	// sets timestamping of statuses on or off
	bool display_timestamp;

	// position where the ":" appears
	uint32_t divider_position;
	
	// id to row translations
	uint32_t message_id_counter;
	std::list<std::pair<uint32_t, uint32_t> > translation_table;

	// scroll render buffer -- internal routine also updates translation table
	void scroll_render_buffer(uint32_t rows_to_scroll);

#else

public:
	
	enhanced_textbox_t() {}
	~enhanced_textbox_t() {}

	inline virtual void write(const void* message, ...) {}

	inline uint32_t write_lhs(const void* message, ...) { return 0; }
	inline void write_rhs(uint32_t lhs_id, const void* message, ...) {}
	
	inline void set_divider_position(uint32_t divider_position) {}
	inline void set_timestamp_display(bool state) {}

private:
	
	// disabled copy constructor and assignment operator
	enhanced_textbox_t(const enhanced_textbox_t& original) {}
	enhanced_textbox_t& operator=(const enhanced_textbox_t& original) {}

#endif

};

// constrained textbox definitions
class constrained_textbox_t : public text_panel
{

#ifndef NO_GUI

public:

	constrained_textbox_t();
	~constrained_textbox_t();

	virtual void write(const void* message, ...);
	void write(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...); // override the default other write method

private:
	
	// disable copy constructor and assignment operator
	constrained_textbox_t(const constrained_textbox_t& original);
	constrained_textbox_t& operator=(const constrained_textbox_t& original);

	bool unwriteable;

#else

public:
	constrained_textbox_t() {};
	virtual ~constrained_textbox_t() {};

	inline virtual void write(const void* message, ...) {};
	inline void write(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...) {};

private:
	constrained_textbox_t(const constrained_textbox_t& original) {};
	constrained_textbox_t& operator=(const constrained_textbox_t& original) {};

#endif
};

// progress bar definitions
class progress_bar_t : public textbox_t
{
#ifndef NO_GUI

public:
	progress_bar_t();

	// use default copy constructor, assignment operator and destructor
	inline void set_numeric_display(bool display_numbers, bool show_percentage_instead_of_raw_value) { display_numerics = display_numbers; display_percentage = show_percentage_instead_of_raw_value; }
	inline void set_raw_value_field_width(uint32_t width) { raw_value_field_width = width; render(); }
	inline void set_progress_bar_fill_char(uint8_t fill_char) { progress_bar_fill_char = fill_char; render(); }
	inline void set_progress_bar_color(uint8_t filled_color, uint8_t background_color) {progress_bar_fill_color = filled_color; progress_bar_background_color = background_color; render(); }
	inline void set_progress_bar_text_color(uint8_t text_color) { progress_bar_text_color = text_color; }
	inline void set_progress(double current_raw_value) {current_value = current_raw_value; render(); }
	inline void set_max_progress_value(double max_raw_units) { max_progress_value = max_raw_units; render(); }

private:

	// copy constructor and assignment operator disabled
	progress_bar_t(const progress_bar_t& original);
	progress_bar_t& operator=(const progress_bar_t& original);

	void render();

	bool display_numerics;
	bool display_percentage;
	uint32_t raw_value_field_width;

	double max_progress_value, current_value;
	uint8_t progress_bar_fill_char;
	uint8_t progress_bar_fill_color, progress_bar_background_color;
	uint8_t progress_bar_text_color;

#else
public:
	progress_bar_t() {}

	// use default copy constructor, assignment operator and destructor
	inline void set_numeric_display(bool display_numbers, bool show_percentage_instead_of_raw_value) {}
	inline void set_raw_value_field_width(uint32_t width) {}
	inline void set_progress_bar_fill_char(uint8_t fill_char) {}
	inline void set_progress_bar_color(uint8_t filled_color, uint8_t background_color) {}
	inline void set_progress_bar_text_color(uint8_t text_color) {}
	inline void set_progress(double current_raw_value) {}
	inline void set_max_progress_value(double max_raw_units) {}

private:

	// copy constructor and assignment operator disabled
	progress_bar_t(const progress_bar_t& original) {}
	progress_bar_t& operator=(const progress_bar_t& original) {}

#endif

};


// input textbox definitions
class input_textbox_t : public input_panel
{
#ifndef NO_GUI

public:

	input_textbox_t();
	~input_textbox_t();

	void get_keypress(uint8_t* ascii_value, uint16_t* special_value);
	void get_line(uint8_t* buffer, uint32_t buffer_size);

protected:
	
	// mouse events -- ignored by textbox processor
	inline virtual void process_click(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mousemove(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mouseout() {};

	// keyboard events
	virtual void process_keypress(uint8_t ascii_value, uint16_t special_value);

	// refresh events
	inline virtual void on_focus() {};
	inline virtual void off_focus() {};

private:

	input_textbox_t(const input_textbox_t& original);
	input_textbox_t& operator= (const input_textbox_t& original);

	pthread_mutex_t keypress_lock;
	pthread_cond_t keypress_cond;
	std::list<std::pair<uint8_t, uint16_t> >keypress_buffer;

#else
public:

	input_textbox_t() {}
	~input_textbox_t(){}

	inline void get_keypress(uint8_t* ascii_value, uint16_t* special_value) {}
	inline void get_line(uint8_t* buffer, uint32_t buffer_size) { while(1) { cu_improved_sleep(1.0);}}

protected:
	
	// mouse events -- ignored by textbox processor
	inline virtual void process_click(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mousemove(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mouseout() {};

	// keyboard events
	inline virtual void process_keypress(uint8_t ascii_value, uint16_t special_value) {}

	// refresh events
	inline virtual void on_focus() {};
	inline virtual void off_focus() {};

private:

	input_textbox_t(const input_textbox_t& original) {}
	input_textbox_t& operator= (const input_textbox_t& original) {}

#endif

};

// input menu definitions
class input_menu_t : public input_panel
{
#ifndef NO_GUI

public:
	input_menu_t();
	~input_menu_t();

	void set_selector_color(uint8_t selector_composite_color);
	void add_menu_item(const void* item_descriptor);
	uint32_t get_menu_input();

protected:

	// mouse events -- ignored by menu processor for now
	inline virtual void process_click(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mousemove(uint32_t rel_x, uint32_t rel_y) {};
	inline virtual void process_mouseout() {};

	// keyboard events
	virtual void process_keypress(uint8_t ascii_value, uint16_t special_value);

	// refresh events
	virtual void on_focus();
	virtual void off_focus();

private:

	input_menu_t(const input_menu_t& original);
	input_menu_t& operator= (const input_menu_t& original);

	uint8_t selector_color;
	
	uint32_t number_of_items;
	char** menu_items;
	uint32_t previous_selection;

	uint32_t menu_render_row;
	output_char_t* backup_buffer;

	// buffer keypresses here
	pthread_mutex_t keypress_lock;
	pthread_cond_t keypress_cond;
	std::list<std::pair<uint8_t, uint16_t> >keypress_buffer;
#else

public:
	input_menu_t() {}
	~input_menu_t() {}

	inline void set_selector_color(uint8_t selector_composite_color) {}
	inline void add_menu_item(const void* item_descriptor) {}
	inline uint32_t get_menu_input() { return 0; }

private:

	input_menu_t(const input_menu_t& original) {}
	input_menu_t& operator= (const input_menu_t& original) {}

#endif

};

class display_controller
{
#ifndef NO_GUI

public:
	static void startup();
	static void shutdown();

	// functions to set up screen
	static void set_screen_resolution(uint32_t max_rows, uint32_t max_columns);
	static void get_screen_resolution(uint32_t* max_rows, uint32_t* max_columns);
	static void set_screen_title(const void* title);

	// panel registration
	static void register_output_panel(text_panel* panel);
	static void unregister_output_panel(text_panel* panel);
	static void register_input_panel(input_panel* panel);
	static void unregister_input_panel(input_panel* panel);

	// automatic depth assignment
	static int32_t z_depth_auto_assignment();

	// write container access
	inline static void clear() { background_textbox.clear(); }
	static void set_color(uint8_t composite_color);
	static void write(const void* message, ...);
	static void write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...);

	static uint8_t get_color() { return background_textbox.get_color(); }

	// display rendered frames to screen
	static void repaint();
	
	// allow classes to update focus if the input list changes
	static void signal_list_change();

	// allow classes to update focus if tab is pressed
	static void update_focus_on_tab();

	// allow classes to center focus on themselves
	static void update_focus_on_list_change();

private:

	// screen metadata
	static pthread_mutex_t display_lock;

#ifdef _WIN32
	static HANDLE hIn, hOut, hError;
#endif
	static uint32_t screen_rows, screen_columns;
	static output_char_t* render_buffer;
	static output_char_t* output_buffer;

	static int32_t z_depth_counter;
	static pthread_mutex_t panel_list_lock;
	static std::list<text_panel*> output_text_panels;
	static textbox_t background_textbox;

	static pthread_mutex_t input_panel_list_lock;
	static std::list<input_panel*> input_panel_list;
	static input_panel* current_focus;

	// flags for input change
	static pthread_mutex_t input_event_lock;
	static pthread_cond_t input_event_cond;
	static bool input_panel_list_changed;
	static std::list<input_event_t> input_event_list;

	// refresh interval
	static const double DISPLAY_REFRESH_INTERVAL;
	static struct timeval last_refresh_time;
	static bool refresh_signalled;
	static pthread_mutex_t refresh_signal_lock;
	static pthread_cond_t refresh_signal_cond;

	// functions come here
	static void hide_system_cursor();

	// thread entrypoints
	static void* periodic_refresh_thread(void* arguments);
	static void* input_reader_thread(void* arguments);
	static void* focus_arbitration_thread(void* arguments);
	static void* repaint_thread(void* arguments);
	
	static bool validate_current_focus_no_lock();

	static void update_focus_on_mouse_click(const input_event_t& mouse_event);
	
	static void arbitrate_focus();

#ifndef _WIN32
	// function for LINUX to setup colors
	static void set_active_color(uint8_t composite_color);
	static void setup_colors();
#endif


#else
public:
	// NO GUI function stubs
	inline static void startup() {}
	inline static void shutdown() {}

	// functions to set up screen
	inline static void set_screen_resolution(uint32_t max_rows, uint32_t max_columns) {}
	inline static void get_screen_resolution(uint32_t* max_rows, uint32_t* max_columns) {}
	inline static void set_screen_title(const void* title) {}

	// panel registration
	inline static void register_output_panel(text_panel* panel) {}
	inline static void unregister_output_panel(text_panel* panel) {}
	inline static void register_input_panel(input_panel* panel) {}
	inline static void unregister_input_panel(input_panel* panel) {}

	// automatic depth assignment
	inline static int32_t z_depth_auto_assignment() { return 0; }

	// write container access
	inline static void clear() {}
	inline static void set_color(uint8_t composite_color) {}
	inline static void write(const void* message, ...) {}
	inline static void write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...) {}

	// display rendered frames to screen
	inline static void repaint() {}
	
	// allow classes to update focus if the input list changes
	inline static void signal_list_change() {}

	// allow classes to update focus if tab is pressed
	inline static void update_focus_on_tab() {}

	// allow classes to center focus on themselves
	inline static void update_focus_on_list_change() {}

#endif


};


#endif

