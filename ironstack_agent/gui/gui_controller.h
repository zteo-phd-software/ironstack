#pragma once

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <ncurses.h>
#include "gui_defs.h"
#include "gui_component.h"
using namespace std;
using namespace gui;

// gui controller class
namespace gui {
class gui_controller : public enable_shared_from_this<gui_controller> {
public:

	// default constructor and destructor
	gui_controller();
	~gui_controller();

	// init and shutdown functions
	bool init();
	void shutdown();

	// refresh control
	void set_refresh_interval(uint32_t interval_ms);
	void refresh();

	// display component registration
	void register_gui_component(const shared_ptr<gui_component>& component);
	void unregister_gui_component(const shared_ptr<gui_component>& component);

	// input component registration and control
	void register_input_component(const shared_ptr<input_component>& component);
	void unregister_input_component(const shared_ptr<input_component>& component);
	void set_enable(const shared_ptr<input_component>& component, bool status);
	void set_focus(const shared_ptr<input_component>& component);
	void relinquish_focus(const shared_ptr<input_component>& component);

	// getter functions
	vec2d get_screen_resolution() const;
	shared_ptr<input_component> get_focus_component();

	// static functions. associate thread output with a component
	// default value of thread::id() associates all other threads and provides a default output component
	// for threads that are not associated
	static void associate_thread_with_component(const thread::id& id, const shared_ptr<gui_component>& component);
	static void associate_local_thread_with_component(const shared_ptr<gui_component>& component);

	// display functions that send local thread output to the appropriate gui component
	static void printf(const char* fmt, ...);
	static void printf(const vec2d& location, const char* fmt, ...);

private:

	// copy constructor and assignment operator disabled
	gui_controller(const gui_controller& other) = delete;
	gui_controller& operator=(const gui_controller& other) = delete;

	// internal state
	mutex              controller_lock;
	condition_variable controller_cond;
	bool               initialized;
	uint32_t           refresh_interval_ms;
	vec2d              screen_dimensions;
	thread             render_tid;
	thread             input_tid;
	WINDOW*            staging_pad;
	WINDOW*            display_window;

	list<weak_ptr<gui_component>>     display_components;
	list<weak_ptr<input_component>>   input_components;
	int                               current_focus_value;
	weak_ptr<input_component>         current_focus_component;

	// static variables used to map output to display components
	static mutex print_mappings_lock;
	static map<thread::id, weak_ptr<gui_component>> print_mappings;
	static weak_ptr<gui_component> default_output_component;

	// thread entrypoints
	void render_entrypoint();
	void input_entrypoint();

	// helper functions
	list<shared_ptr<input_component>> get_sorted_input_components_unsafe();

	// used to find candidate components to apply focus/blur to
	// repick=false tries to preserve the current component's focus if possible
	pair<shared_ptr<input_component>, shared_ptr<input_component>> arbitrate_focus_unsafe(bool repick);

	// setup ncurses colorpair information
	void setup_colors();

};};
