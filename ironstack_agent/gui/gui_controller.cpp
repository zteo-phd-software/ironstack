#include "gui_controller.h"
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>

// static variables here
mutex gui_controller::print_mappings_lock;
map<thread::id, weak_ptr<gui_component>> gui_controller::print_mappings;
weak_ptr<gui_component> gui_controller::default_output_component;

// constructor
gui_controller::gui_controller() {
	initialized = false;
	refresh_interval_ms = 500;
	staging_pad = nullptr;
	display_window = nullptr;
	current_focus_value = 0;
}

// destructor
gui_controller::~gui_controller() {
	shutdown();
}

// initializes the display controller
bool gui_controller::init() {
	lock_guard<mutex> g(controller_lock);
	if (initialized) return true;

	initscr();
	start_color();
	setup_colors();
	keypad(stdscr, TRUE);
	curs_set(0);
	noecho();
	timeout(1000);
	getmaxyx(stdscr, screen_dimensions.y, screen_dimensions.x);

	staging_pad = newpad(screen_dimensions.y, screen_dimensions.x);
	display_window = newwin(screen_dimensions.y, screen_dimensions.x, 0, 0);
	if (staging_pad == nullptr || display_window == nullptr) {
		assert(false && "unable to allocate staging/display pad.");
		abort();
	}

	initialized = true;
	render_tid = thread(&gui::gui_controller::render_entrypoint, this);
	input_tid = thread(&gui::gui_controller::input_entrypoint, this);
	
	return true;
}

// shuts down the display controller
void gui_controller::shutdown() {
	if (!initialized) return;

	// signal render thread to stop and join the thread
	initialized = false;
	{
		unique_lock<mutex> g(controller_lock);
		controller_cond.notify_all();
	}

	render_tid.join();
	if (staging_pad != nullptr) delwin(staging_pad);
	if (display_window != nullptr) delwin(display_window);

	staging_pad = nullptr;
	display_window = nullptr;
	endwin();
}

// sets the automatic redraw interval on the display controller
void gui_controller::set_refresh_interval(uint32_t interval_ms) {
	unique_lock<mutex> g(controller_lock);
	refresh_interval_ms = interval_ms;
	controller_cond.notify_all();
}

// forces a redraw on all components
void gui_controller::refresh() {
	unique_lock<mutex> g(controller_lock);
	controller_cond.notify_all();
}

// registers a component for display purposes
void gui_controller::register_gui_component(const shared_ptr<gui_component>& component) {
	if (component == nullptr) return;

	// erase stale entries; look for duplicates
	bool found = false;
	lock_guard<mutex> g(controller_lock);
	auto iterator = display_components.begin();
	while (iterator != display_components.end()) {
		auto current = iterator->lock();
		if (current == nullptr) {
			iterator = display_components.erase(iterator);
		} else {
			if (current == component) {
				found = true;
			}
			++iterator;
		}
	}

	// no duplicates; add the entry
	if (!found) {
		display_components.emplace_back(component);
		component->set_controller(shared_from_this());
	}
}

// unregisters a displayable component (also clears other invalid pointers)
void gui_controller::unregister_gui_component(const shared_ptr<gui_component>& component) {
	if (component == nullptr) return;

	// remove the gui component from local display list
	{
		lock_guard<mutex> g(controller_lock);
		auto iterator = display_components.begin();
		while (iterator != display_components.end()) {
			shared_ptr<gui_component> current = iterator->lock();
			if (current == nullptr || current == component) {
				iterator = display_components.erase(iterator);
			} else {
				++iterator;
			}
		}
	}

	// remove thread output mappings that are stale or is the component to unregister
	{
		lock_guard<mutex> g(print_mappings_lock);
		auto iterator = print_mappings.begin();
		while (iterator != print_mappings.end()) {
			auto current = iterator->second.lock();
			if (current == nullptr || current == component) {
				iterator = print_mappings.erase(iterator);
			} else {
				++iterator;
			}
		}

		auto default_output = default_output_component.lock();
		if (default_output == nullptr || default_output == component) {
			shared_ptr<gui_component> dummy_null;
			default_output_component = dummy_null;
		}
	}
}

// returns 1. component to apply focus, 2. component to apply blur
pair<shared_ptr<input_component>, shared_ptr<input_component>> gui_controller::arbitrate_focus_unsafe(bool repick) {

	list<shared_ptr<input_component>> candidate_components;
	shared_ptr<input_component> component_with_focus = current_focus_component.lock();
	bool found_component_with_focus = false;

	// purge stale components; make a note of which components are enabled for input
	auto iterator = input_components.begin();
	while (iterator != input_components.end()) {
		auto current = iterator->lock();
		if (current == nullptr) {
			iterator = input_components.erase(iterator);
		} else {
			if (current->enabled) {
				candidate_components.push_back(current);
				if (current == component_with_focus) {
					found_component_with_focus = true;
				}
			}
			++iterator;
		}
	}

	// if there are no candidate components, but something still seems to be active, disable it
	if (candidate_components.empty() && component_with_focus != nullptr) {
		current_focus_value = 0;
		current_focus_component = shared_ptr<input_component>();
		return pair<shared_ptr<input_component>, shared_ptr<input_component>>(nullptr, component_with_focus);
	}

	// if there are no candidate components, then there's nothing to do
	if (candidate_components.empty()) {
		current_focus_value = 0;
		return make_pair<shared_ptr<input_component>, shared_ptr<input_component>>(nullptr, nullptr);
	}

	// if a repick isn't called for and the current focus is still valid, then return
	if (!repick && found_component_with_focus) {
		return make_pair<shared_ptr<input_component>, shared_ptr<input_component>>(nullptr, nullptr);

	// if a repick is required, look for the next component with greater than (or equal) input sequence
	} else {

		int new_focus_candidate_value;
		shared_ptr<input_component> new_focus_candidate;

		// if there is currently a component with focus, the search should begin from its focus value
		// otherwise, begin the search from the stored focus value
		if (component_with_focus != nullptr) {
			new_focus_candidate_value = component_with_focus->input_sequence;
		} else {
			new_focus_candidate_value = current_focus_value;
		}

		// search for the next greater input sequence
		for (const auto& candidate : candidate_components) {
			if (candidate != component_with_focus && candidate->input_sequence >= new_focus_candidate_value) {
				return pair<shared_ptr<input_component>, shared_ptr<input_component>>(candidate, component_with_focus);
			}
		}

		// greater input sequence couldn't be found; wraparound and find component with smallest focus value
		new_focus_candidate_value = candidate_components.front()->input_sequence;
		new_focus_candidate = candidate_components.front();
		for (const auto& candidate : candidate_components) {
			if (candidate->input_sequence < new_focus_candidate_value) {
				new_focus_candidate_value = candidate->input_sequence;
				new_focus_candidate = candidate;
			}
		}

		// if the wraparound component is still the same as the current focused component, then there's nothing to do (the candidate list size must have been 1)
		if (new_focus_candidate == component_with_focus) {
			return make_pair<shared_ptr<input_component>, shared_ptr<input_component>>(nullptr, nullptr);

		// focus has changed
		} else {
			return pair<shared_ptr<input_component>, shared_ptr<input_component>>(new_focus_candidate, component_with_focus);
		}
	}
}

// registers a component for input purposes
void gui_controller::register_input_component(const shared_ptr<input_component>& component) {
	if (component == nullptr) return;
	component->set_controller(shared_from_this());
	shared_ptr<input_component> component_to_focus, component_to_blur;

	// prevent multiple registrations
	{
		lock_guard<mutex> g(controller_lock);
		for (const auto& one_component : input_components) {
			if (one_component.lock() == component) return;
		}
		input_components.emplace_back(component);

		auto result = arbitrate_focus_unsafe(false);
		component_to_focus = result.first;
		component_to_blur = result.second;
	}

	// perform callbacks
	component->on_register();

	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
}

// unregisters an input component (also clears invalid pointers and fixes up focus value)
void gui_controller::unregister_input_component(const shared_ptr<input_component>& component) {
	if (component == nullptr) return;

	shared_ptr<input_component> component_to_focus, component_to_blur;
	shared_ptr<input_component> component_with_focus = current_focus_component.lock();
	{
		lock_guard<mutex> g(controller_lock);

		// remove component from regular input component list
		auto iterator2 = input_components.begin();
		while (iterator2 != input_components.end()) {
			auto current = iterator2->lock();
			if (current == nullptr || current == component) {
				iterator2 = input_components.erase(iterator2);
			} else {
				++iterator2;
			}
		}

		// if the component being removed is the one in focus, time to alter focus
		if (component == component_with_focus) {
			auto result = arbitrate_focus_unsafe(true);
			component_to_focus = result.first;
			component_to_blur = result.second;
		}
	}

	// perform callbacks
	component->on_unregister();

	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
}

// enables or disables a component for input
void gui_controller::set_enable(const shared_ptr<input_component>& component, bool status) {
	if (component == nullptr) return;

	shared_ptr<input_component> component_to_focus, component_to_blur;
	{
		lock_guard<mutex> g(controller_lock);

		// if current component is going to be disabled but it is currently in focus, find the next component to give focus to
		bool repick = false;
		if (!status && current_focus_component.lock() == component && current_focus_value == component->input_sequence) {
			repick = true;
		}

		auto result = arbitrate_focus_unsafe(repick);
		component_to_focus = result.first;
		component_to_blur = result.second;
	}

	// perform callbacks
	component->on_unregister();

	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
}

// shifts focus to the given component
void gui_controller::set_focus(const shared_ptr<input_component>& component) {
	if (component == nullptr) return;
	bool component_is_enabled = component->enabled;

	shared_ptr<input_component> component_to_focus, component_to_blur;
	{
		lock_guard<mutex> g(controller_lock);

		// don't refocus again if the component already has focus
		shared_ptr<input_component> component_with_focus = current_focus_component.lock();
		if (component_with_focus == component && current_focus_value == component->input_sequence && component_is_enabled) {
			return;
		}

		// otherwise, simulate a repick to get the active component to blur
		auto result = arbitrate_focus_unsafe(true);
		component_to_focus = result.first;
		component_to_blur = result.second;
	}

	// we can only give focus to enabled components
	if (component_is_enabled) {
		component_to_focus = component;
	}

	// perform callbacks
	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
}

// gives up focus on a GUI component (if it is currently in focus). if focus
// is relinquished, this component is blurred and the next component in the GUI
// input chain receives focus
void gui_controller::relinquish_focus(const shared_ptr<input_component>& component) {
	if (component == nullptr) return;

	shared_ptr<input_component> component_to_focus, component_to_blur;

	// only really relinquish focus if the given component is actually in focus
	{
		lock_guard<mutex> g(controller_lock);
		shared_ptr<input_component> component_with_focus = current_focus_component.lock();

		// repick if component is in focus, otherwise don't repick
		auto result = arbitrate_focus_unsafe(component_with_focus == component && current_focus_value == component->input_sequence);
		component_to_focus = result.first;
		component_to_blur = result.second;
	}

	// perform callbacks
	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
}

// returns the screen resolution
vec2d gui_controller::get_screen_resolution() const {
	int max_x, max_y;
	getmaxyx(stdscr, max_y, max_x);
	return vec2d(max_x, max_y);
}

// returns a pointer to the current component that has input focus
shared_ptr<input_component> gui_controller::get_focus_component() {

	shared_ptr<input_component> component_to_focus, component_to_blur;
	{
		lock_guard<mutex> g(controller_lock);
		auto result = arbitrate_focus_unsafe(false);
		component_to_focus = result.first;
		component_to_blur = result.second;
	}

	// perform callbacks
	if (component_to_blur != nullptr) {
		component_to_blur->on_blur();
	}

	if (component_to_focus != nullptr) {
		current_focus_value = component_to_focus->input_sequence;
		current_focus_component = component_to_focus;
		component_to_focus->on_focus();
	}

	refresh();
	return current_focus_component.lock();
}

// associate thread output with a component
void gui_controller::associate_thread_with_component(const thread::id& id, const shared_ptr<gui_component>& component) {
	lock_guard<mutex> g(print_mappings_lock);
	thread::id default_id;

	if (id == default_id) {
		default_output_component = component;
	} else {
		print_mappings[id] = component;
	}
}

// associate local thread with output component
void gui_controller::associate_local_thread_with_component(const shared_ptr<gui_component>& component) {
	associate_thread_with_component(this_thread::get_id(), component);
}

// prints to the output depending on thread ID
void gui_controller::printf(const char* fmt, ...) {

	// lookup GUI component
	thread::id thread_id = this_thread::get_id();
	shared_ptr<gui_component> component;
	{
		lock_guard<mutex> g(print_mappings_lock);
		auto iterator = print_mappings.find(thread_id);
		if (iterator != print_mappings.end()) {
			component = iterator->second.lock();
		} else {
			component = default_output_component.lock();
		}
	}

	// don't do any printf if component is invalid
	if (component == nullptr) return;
	
	// generate the printed buffer
	char message_buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(message_buf, sizeof(message_buf), fmt, args);
	va_end(args);

	// perform slow write outside critical section
	component->printf("%s", message_buf);
	component->commit_output();
	
	// update the output
	shared_ptr<gui_controller> controller = component->get_controller();
	if (controller != nullptr) controller->refresh();
}

// prints to a specific position on the screen
void gui_controller::printf(const vec2d& location, const char* fmt, ...) {

	// lookup GUI component
	thread::id thread_id = this_thread::get_id();
	shared_ptr<gui_component> component;
	{
		lock_guard<mutex> g(print_mappings_lock);
		auto iterator = print_mappings.find(thread_id);
		if (iterator != print_mappings.end()) {
			component = iterator->second.lock();
		} else {
			component = default_output_component.lock();
		}
	}

	// don't do any printf if component is invalid
	if (component == nullptr) return;

	// generate the printed buffer
	char message_buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(message_buf, sizeof(message_buf), fmt, args);
	va_end(args);

	// perform slow write outside critical section
	component->printf(location, "%s", message_buf);
	component->commit_output();

	// update the output
	shared_ptr<gui_controller> controller = component->get_controller();
	if (controller != nullptr) controller->refresh();
}

// this function does all the magic for display
void gui_controller::render_entrypoint() {
	unique_lock<mutex> controller_lock(this->controller_lock);
	while(1) {
		controller_cond.wait_for(controller_lock, chrono::milliseconds(refresh_interval_ms));
		if (!initialized) return;

		// eliminate stale entries, collect shared pointers to active components
		list<shared_ptr<gui_component>> current_components;
		auto iterator = display_components.begin();
		while(iterator != display_components.end()) {
			auto current = iterator->lock();
			if (current == nullptr) {
				iterator = display_components.erase(iterator);
			} else {
				current_components.push_back(current);
				++iterator;
			}
		}

		// perform a render as required
		for (auto& component : current_components) {
			component->render();
		}

		// z-sort the components
		current_components.sort(
			[](const shared_ptr<gui_component>& lhs,
			const shared_ptr<gui_component>& rhs) {
				return lhs->get_z_position() > rhs->get_z_position();
			}
		);

		// render back to front
		pnoutrefresh(staging_pad, 0,0,0,0, screen_dimensions.y, screen_dimensions.x);
		for (auto& component : current_components) {
			vec2d origin;

			lock_guard<mutex> g(component->active_lock);
		
			if (!component->active_state.visible) {
				continue;
			}

			// compute origin
			origin = component->active_state.origin;
			switch (component->active_state.alignment) {
				case alignment_t::TOP_LEFT:
					break;
				case alignment_t::TOP_CENTER:
					origin.x -= component->active_state.dimensions.x/2;
					break;
				case alignment_t::TOP_RIGHT:
					origin.x -= component->active_state.dimensions.x;
					break;
				case alignment_t::MIDDLE_LEFT:
					origin.y -= component->active_state.dimensions.y/2;
					break;
				case alignment_t::MIDDLE_CENTER:
					origin.x -= component->active_state.dimensions.x/2;
					origin.y -= component->active_state.dimensions.y/2;
					break;
				case alignment_t::MIDDLE_RIGHT:
					origin.x -= component->active_state.dimensions.x;
					origin.y -= component->active_state.dimensions.y/2;
					break;
				case alignment_t::BOTTOM_LEFT:
					origin.y -= component->active_state.dimensions.y;
					break;
				case alignment_t::BOTTOM_CENTER:
					origin.x -= component->active_state.dimensions.x/2;
					origin.y -= component->active_state.dimensions.y;
					break;
				case alignment_t::BOTTOM_RIGHT:
					origin.x -= component->active_state.dimensions.x;
					origin.y -= component->active_state.dimensions.y;
					break;
			}

			// trivial visibility checks
			vec2d dimensions = component->active_state.dimensions;
			if ((origin.x + dimensions.x <= 0) 
				|| (origin.y + dimensions.y <= 0)
				|| (origin.x >= screen_dimensions.x)
				|| (origin.y >= screen_dimensions.y)) {
				continue;
			}

			// object must be partially visible here, so get the clipping coordinates
			vec2d clip_origin;
			vec2d clip_dimensions;
			clip_origin.x = (origin.x >= 0 ? 0 : -origin.x);	// where to start copying src win from
			clip_origin.y = (origin.y >= 0 ? 0 : -origin.y);	// where to start copying src win from
			clip_dimensions.x = (dimensions.x - clip_origin.x + origin.x >= screen_dimensions.x ? screen_dimensions.x - origin.x : dimensions.x - clip_origin.x);
			clip_dimensions.y = (dimensions.y - clip_origin.y + origin.y >= screen_dimensions.y ? screen_dimensions.y - origin.y : dimensions.y - clip_origin.y);

			// copy from component to pad
			pnoutrefresh(component->active_pad, clip_origin.y, clip_origin.x, origin.y, origin.x, origin.y+clip_dimensions.y-1, origin.x+clip_dimensions.x-1);
		}

		// copy staging pad to display
		doupdate();
	}
}

// thread that handles input
void gui_controller::input_entrypoint() {

	int keypress;

	while(initialized) {

		// get keypress and filter
		keypress = getch();
		if (keypress == ERR) continue;
		
		keystroke current_key;

		// handle special keys here
		switch(keypress) {
			case KEY_DOWN:
				current_key.special_key = special_key_t::ARROW_DOWN;
				break;
			case KEY_UP:
				current_key.special_key = special_key_t::ARROW_UP;
				break;
			case KEY_LEFT:
				current_key.special_key = special_key_t::ARROW_LEFT;
				break;
			case KEY_RIGHT:
				current_key.special_key = special_key_t::ARROW_RIGHT;
				break;
			case KEY_HOME:
				current_key.special_key = special_key_t::HOME;
				break;
			case KEY_NPAGE:
				current_key.special_key = special_key_t::PAGE_DOWN;
				break;
			case KEY_PPAGE:
				current_key.special_key = special_key_t::PAGE_UP;
				break;
			case KEY_END:
				current_key.special_key = special_key_t::END;
				break;
			case KEY_DC:
				current_key.special_key = special_key_t::DELETE;
				break;
			case KEY_BACKSPACE:
				current_key.ascii_value = '\b';
				break;
			case KEY_STAB:
				current_key.ascii_value = '\t';
				break;

			// ordinary ascii values
			default:
				current_key.ascii_value = (uint8_t) keypress;
		}
		if (!current_key.is_valid()) continue;

		shared_ptr<input_component> component_with_focus = get_focus_component();
		if (component_with_focus != nullptr) {
			component_with_focus->process_keypress(current_key);
		}

/*
		// look for the focus candidate to send the keystroke to
		list<shared_ptr<input_component>> current_input_components;
		{
			lock_guard<mutex> g(controller_lock);
		
			// eliminate stale entries and compile list of input components
			current_input_components = get_sorted_input_components_unsafe();
		}

		// look for component with focus value
		shared_ptr<input_component> component_to_focus;
		if (current_input_components.empty()) goto done;
		for (const auto& component : current_input_components) {
			if (component->input_sequence == current_focus_value) {
				component->process_keypress(current_key);
				goto done;
			}
		}

		// find the next component with focus value
		for (const auto& component : current_input_components) {
			if (component->input_sequence >= current_focus_value) {
				component_to_focus = component;
				break;
			}
		}

		if (component_to_focus == nullptr) {
			component_to_focus = current_input_components.front();
		}
		current_focus_value = component_to_focus->input_sequence;

		// apply focus
		component_to_focus->on_focus();
		component_to_focus->process_keypress(current_key);
*/
	}
}

// removes stale entries and returns a list of input components that are valid
list<shared_ptr<input_component>> gui_controller::get_sorted_input_components_unsafe() {

	auto iterator = input_components.begin();
	list<shared_ptr<input_component>> result;
	while (iterator != input_components.end()) {
		auto current = iterator->lock();
		if (current == nullptr) {
			iterator = input_components.erase(iterator);
		} else {
			result.push_back(current);
			++iterator;
		}
	}

	// sort the results by input sequence
	result.sort(
		[](const shared_ptr<input_component>& lhs,
		const shared_ptr<input_component>& rhs) {
			return lhs->input_sequence < rhs->input_sequence;
		}
	);

	return result;
}

// setup ncurses color pairs
void gui_controller::setup_colors() {
  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(6, COLOR_YELLOW, COLOR_BLACK);
  init_pair(7, COLOR_WHITE, COLOR_BLACK);

  init_pair(8, COLOR_BLACK, COLOR_BLUE);
  init_pair(9, COLOR_BLUE, COLOR_BLUE);
  init_pair(10, COLOR_GREEN, COLOR_BLUE);
  init_pair(11, COLOR_CYAN, COLOR_BLUE);
  init_pair(12, COLOR_RED, COLOR_BLUE);
  init_pair(13, COLOR_MAGENTA, COLOR_BLUE);
  init_pair(14, COLOR_YELLOW, COLOR_BLUE);
  init_pair(15, COLOR_WHITE, COLOR_BLUE);

  init_pair(16, COLOR_BLACK, COLOR_GREEN);
  init_pair(17, COLOR_BLUE, COLOR_GREEN);
  init_pair(18, COLOR_GREEN, COLOR_GREEN);
  init_pair(19, COLOR_CYAN, COLOR_GREEN);
  init_pair(20, COLOR_RED, COLOR_GREEN);
  init_pair(21, COLOR_MAGENTA, COLOR_GREEN);
  init_pair(22, COLOR_YELLOW, COLOR_GREEN);
  init_pair(23, COLOR_WHITE, COLOR_GREEN);

  init_pair(24, COLOR_BLACK, COLOR_CYAN);
  init_pair(25, COLOR_BLUE, COLOR_CYAN);
  init_pair(26, COLOR_GREEN, COLOR_CYAN);
  init_pair(27, COLOR_CYAN, COLOR_CYAN);
  init_pair(28, COLOR_RED, COLOR_CYAN);
  init_pair(29, COLOR_MAGENTA, COLOR_CYAN);
  init_pair(30, COLOR_YELLOW, COLOR_CYAN);
  init_pair(31, COLOR_WHITE, COLOR_CYAN);

  init_pair(32, COLOR_BLACK, COLOR_RED);
  init_pair(33, COLOR_BLUE, COLOR_RED);
  init_pair(34, COLOR_GREEN, COLOR_RED);
  init_pair(35, COLOR_CYAN, COLOR_RED);
  init_pair(36, COLOR_RED, COLOR_RED);
  init_pair(37, COLOR_MAGENTA, COLOR_RED);
  init_pair(38, COLOR_YELLOW, COLOR_RED);
  init_pair(39, COLOR_WHITE, COLOR_RED);

  init_pair(40, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(41, COLOR_BLUE, COLOR_MAGENTA);
  init_pair(42, COLOR_GREEN, COLOR_MAGENTA);
  init_pair(43, COLOR_CYAN, COLOR_MAGENTA);
  init_pair(44, COLOR_RED, COLOR_MAGENTA);
  init_pair(45, COLOR_MAGENTA, COLOR_MAGENTA);
  init_pair(46, COLOR_YELLOW, COLOR_MAGENTA);
  init_pair(47, COLOR_WHITE, COLOR_MAGENTA);

  init_pair(48, COLOR_BLACK, COLOR_YELLOW);
  init_pair(49, COLOR_BLUE, COLOR_YELLOW);
  init_pair(50, COLOR_GREEN, COLOR_YELLOW);
  init_pair(51, COLOR_CYAN, COLOR_YELLOW);
  init_pair(52, COLOR_RED, COLOR_YELLOW);
  init_pair(53, COLOR_MAGENTA, COLOR_YELLOW);
  init_pair(54, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(55, COLOR_WHITE, COLOR_YELLOW);

  init_pair(56, COLOR_BLACK, COLOR_WHITE);
  init_pair(57, COLOR_BLUE, COLOR_WHITE);
  init_pair(58, COLOR_GREEN, COLOR_WHITE);
  init_pair(59, COLOR_CYAN, COLOR_WHITE);
  init_pair(60, COLOR_RED, COLOR_WHITE);
  init_pair(61, COLOR_MAGENTA, COLOR_WHITE);
  init_pair(62, COLOR_YELLOW, COLOR_WHITE);
  init_pair(63, COLOR_WHITE, COLOR_WHITE);
}
