#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <ncurses.h>
#include <panel.h>
#include "gui_defs.h"
using namespace std;
namespace gui {
class gui_controller;

// GUI component metadata. these attributes are inspected by the controller
// and indirectly affect the appearance of the output
class gui_metadata {
public:
	gui_metadata();

	// publicly accessible fields
	bool        visible;		// default: true
	vec2d       origin;			// default: (0,0)
	int         z_position;	// auto assigned. lower/more negative = nearer to screen
	alignment_t alignment;	// default: top left
	vec2d       dimensions;	// default: (0,0)
};

// GUI component draw state. these attributes directly affect the appearance
// of the output, but are not visible or used by the controller
class gui_draw_state {
public:
	gui_draw_state();

	// publicly accessible fields
	color_t     current_color;		// default: FG_WHITE | BG_BLACK
	vec2d       cursor_position;	// default: (0,0)
	color_t     cursor_color;			// default: FG_BLACK | BG_GREEN
	bool        cursor_visible;		// default: false
};


// all GUI components must be created using shared_ptr.
// z depths are auto-assigned starting from value 10000 and it increases
// in value (ie, goes farther behind).
class gui_component {
public:

	// constructor and destructor
	gui_component();
	~gui_component();

	// used to load/save gui components from/to disk (these non-virtual functions
	// only read off the disk and convert to/from key-value pairs). actual
	// parsing is done in the virtual load_parser/save_parser functions.
	bool         load(const string& filename);
	bool         save(const string& filename);

	// set a pointer to the controller (automatically called by the controller
	// on registration
	void         set_controller(const shared_ptr<gui_controller>& controller);
	shared_ptr<gui_controller> get_controller() const;

	// setup object metadata. properties are not changed on display until
	// commit_attributes() is called.
	void         set_visible(bool visible);
	void         set_origin(const vec2d& origin);
	void         set_z_position(int z);	// lower/more negative=nearer to screen
	void         set_alignment(const alignment_t& alignment);
	void         set_dimensions(const vec2d& dimensions);

	// setup object draw state. properties are immediately applied to the back
	// buffer drawing, so any new printfs() will immediately use these settings.
	// output is flushed to display using commit_output().
	void         set_color(const color_t& color);
	void         set_cursor(const vec2d& location);
	void         set_cursor_color(const color_t& color);
	void         set_cursor_visible(bool visible);

	// object output functions -- not applied until commit_output() is called.
	// the virtual functions are defined in the protected section. (these
	// functions merely acquire a lock and call corresponding unsafe functions).
	virtual void clear();
	void         printf(const char* fmt, ...);
	void         printf(const vec2d& location, const char* fmt, ...);
	void         printfc(color_t color, const char* fmt, ...);
	void         printfc(color_t color, const vec2d& location, const char* fmt,
		...);

	// output control. all commits acquire both pending and active locks! don't
	// call them when you have one/both locks acquired. either unlock and use
	// the commit functions, or acquire the remaining lock and call the unsafe
	// commit function.
	virtual void commit_attributes();	// finalizes changes to object metadata.
	virtual void commit_output();			// finalizes changes to object output.
	                                  // acquires both pending and active locks!
	virtual void commit_all();				// finalizes object metadata + output.

	virtual void render();	// called during each controller refresh cycle.
													// can be overriden for lazy evaluation of renders.
													// remember to call commit_output() after rendering
													// or no effect will be observed during refresh.
													// default implementation does nothing.

	// retrieve object state that is being displayed (ie, active state), not the
	// state of the back buffer object
	bool         is_visible() const;
	vec2d        get_origin() const;
	int          get_z_position() const;	// lower/more negative=nearer to screen
	vec2d        get_dimensions() const;

	// retrieve draw state of the object being displayed (ie, active state)
	color_t      get_color() const;
	vec2d        get_cursor() const;
	color_t      get_cursor_color() const;
	bool         is_cursor_visible() const;

	// statically available function to auto assign a depth
	static int   get_next_z_depth();

protected:

	mutable mutex  pending_lock;
	gui_metadata   pending_state;	// state as modified but not committed
	gui_draw_state pending_draw_state;
	WINDOW*        pending_pad;

	mutable mutex  active_lock;
	gui_metadata   active_state;	// state that is presently seen by controller
	gui_draw_state active_draw_state;
	WINDOW*        active_pad;
	bool           needs_redraw;  // means that render() needs to be called
																// before the back buffer is ready to be
																// committed

	weak_ptr<gui_controller> output_controller;

	// override these second stage loader/save functions as necessary
	virtual bool load_parser(const map<string, string>& kv_store);		// this function acquires the pending lock
	virtual bool load_parser_unsafe(const map<string, string>& kv_store);// this function does the actual loading work
	virtual map<string, string> save_parser() const;

	// thread-unsafe version of user-facing functions
	void set_visible_unsafe(bool visible);
	void set_origin_unsafe(const vec2d& origin);
	void set_z_position_unsafe(int z);
	void set_alignment_unsafe(const alignment_t& alignment);
	void set_dimensions_unsafe(const vec2d& dimensions);
	void set_color_unsafe(const color_t& color);
	void set_cursor_unsafe(const vec2d& location);
	void set_cursor_color_unsafe(const color_t& color);
	void set_cursor_visible_unsafe(bool visible);
	void printf_unsafe(const char* fmt, ...);
	void vprintf_unsafe(const char* fmt, va_list argp);
	void printf_unsafe(const vec2d& location, const char* fmt, ...);
	void vprintf_unsafe(const vec2d& location, const char* fmt, va_list argp);
	void printfc_unsafe(color_t color, const char* fmt, ...);
	void vprintfc_unsafe(color_t color, const char* fmt, va_list argp);
	void printfc_unsafe(color_t color, const vec2d& location, const char* fmt,
		...);
	void vprintfc_unsafe(color_t color, const vec2d& location, const char* fmt,
		va_list argp);

	// thread-unsafe version of commit
	void commit_attributes_unsafe();
	void commit_output_unsafe();

	// private helper functions
	void change_color_unsafe(WINDOW* target, uint8_t color);
	void clear_pad_unsafe();
	void multiline_print_unsafe(const char* text);
	void pad_to_eol_unsafe();
	void scroll_unsafe();

	// used for automatically assigning depth values
	static atomic_int last_z_depth;

	// allow gui controller to access active lock, state, pad and needs_redraw
	friend class gui_controller;
};

// describes an input component class
class input_component : public enable_shared_from_this<input_component> {
public:

	// constructor
	input_component();
	~input_component();

	// set a pointer to the controller (automatically called by the controller
	// on registration
	void         set_controller(const shared_ptr<gui_controller>& controller);

	// used to enable/disable eligibility for keystroke events
	void         set_enable(bool enabled);
	bool         is_enabled() const;

	// set the input sequence (lower = earlier in sequence). default=10000.
	// automatically set at instantiation time to latest in sequence.
	void         set_input_sequence(int sequence);
	int          get_input_sequence() const;

	// used to trigger a focus or defocus event
	void         set_focus();
	void         relinquish_focus();

	// called immediately after the object has been registered (default: no-op)
	virtual void on_register();

	// called immediately after the object has been unregistered (default: no-op)
	virtual void on_unregister();

	// called when the object gains focus (default: no-op)
	virtual void on_focus();

	// called when the object loses focus (default: no-op)
	virtual void on_blur();

	// called when a key is available for the current object. must be overriden.
	virtual void process_keypress(const keystroke& key)=0;

	// statically available function to auto assign the latest sequence
	static int   get_next_input_sequence();

protected:

	atomic_bool              enabled;
	atomic_int               input_sequence;
	weak_ptr<gui_controller> input_controller;

	// used for automatically assigning input sequence numbers
	// this sequence number determines the 'tab order' when
	// arbitrating input focus
	static atomic_int        last_input_sequence;

	// allow gui controller to access sequence and modality
	friend class gui_controller;
};
};
