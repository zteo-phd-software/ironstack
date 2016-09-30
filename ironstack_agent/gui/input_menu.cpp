#include <memory.h>
#include <string.h>
#include "gui_controller.h"
#include "input_menu.h"
using namespace gui;

// constructor
input_menu::input_menu() {
	modal = false;
	has_focus = false;
	focus_color = FG_WHITE | BG_BLUE;
	blur_color = FG_GRAY | BG_BLACK;
	scrollbar_color = FG_WHITE | BG_BLACK;
	scrollbar_position_color = FG_WHITE | BG_YELLOW;
	desc_lines = 0;
	option_color = FG_WHITE | BG_RED;
	choice = -1;
	decision_made = false;
	view_range_top = -1;
}

// destructor
input_menu::~input_menu() {}

// set modality
void input_menu::set_modal(bool status) {
	modal = status;
}

// get the modality
bool input_menu::is_modal() const {
	return modal;
}

// set the menu description (optional)
void input_menu::set_description(const string& desc, uint32_t max_lines) {
	lock_guard<mutex> g(pending_lock);
	if (description != desc || desc_lines != max_lines) {
		description = desc;
		desc_lines = max_lines;
		needs_redraw = true;
	}
}

// returns the menu description
string input_menu::get_description() const {
	lock_guard<mutex> g(pending_lock);
	return description;
}

// set the menu focus or blurred color
void input_menu::set_menu_color(const color_t& focus_color_, const color_t& blur_color_) {
	lock_guard<mutex> g(pending_lock);
	if (focus_color != focus_color_ || blur_color != blur_color_) {
		focus_color = focus_color_;
		blur_color = blur_color_;
		needs_redraw = true;
	}
}

// set the option highlight color
void input_menu::set_option_highlight_color(const color_t& color) {
	lock_guard<mutex> g(pending_lock);
	if (option_color != color) {
		option_color = color;
		needs_redraw = true;
	}
}

// set the scrollbar color
void input_menu::set_scrollbar_color(const color_t& bar_color, const color_t& position_color) {
	lock_guard<mutex> g(pending_lock);
	if (scrollbar_color != bar_color || scrollbar_position_color != position_color) {
		scrollbar_color = bar_color;
		scrollbar_position_color = position_color;
		needs_redraw = true;
	}
}

// removes all options
void input_menu::clear_options() {
	lock_guard<mutex> g(pending_lock);
	if (!options.empty()) {
		options.clear();
		choice = -1;
		view_range_top = -1;
		needs_redraw = true;
	}
}

// add an option to the menu
void input_menu::add_option(const string& option) {
	lock_guard<mutex> g(pending_lock);
	if (options.empty()) {
		choice = 0;
		view_range_top = 0;
	}
	options.push_back(option);
	needs_redraw = true;
}

// modifies an option directly within the vector.
// does not perform insertion -- if the index is outside of the vector range,
// nothing will be changed
void input_menu::set_option(int index, const string& option) {
	lock_guard<mutex> g(pending_lock);
	if (index < 0 || index >= (int)options.size()) {
		return;
	} else {
		if (options[index] != option) {
			options[index] = option;
			needs_redraw = true;
		}
	}
}

// returns the number of options in the menu
uint32_t input_menu::get_num_options() const {
	lock_guard<mutex> g(pending_lock);
	return options.size();
}

// returns the option at a given index
string input_menu::get_option(int index) const {
	lock_guard<mutex> g(pending_lock);
	return options[index];
}

// returns all the options in the menu
vector<string> input_menu::get_all_options() const {
	lock_guard<mutex> g(pending_lock);
	return options;
}

// sets the current selection (highlights the choice)
void input_menu::set_current_selection(int choice_) {
	lock_guard<mutex> g(pending_lock);
	set_current_selection_unsafe(choice_);
}

// returns the string corresponding to the currently highlighted item
string input_menu::get_current_selection_string() const {
	lock_guard<mutex> g(pending_lock);
	if (!enabled || choice < 0 || choice >= (int)options.size()) return string();
	return options[choice];
}

// returns the vector index corresponding to the currently highlighted item
int input_menu::get_current_selection() const {
	lock_guard<mutex> g(pending_lock);
	if (!enabled || choice < 0 || choice >= (int)options.size()) return -1;
	return choice;
}

// waits until a selection is made
// timeout_ms : -1 means blocking (default)
// 0 and above: wait for that number of milliseconds
int input_menu::wait_for_decision(int timeout_ms) {

	// automatically enable input if the component was previously disabled
	if (!is_enabled()) set_enable(true);

	// if a decision was made before this function was called, use that decision
	if (decision_made && decision_keystroke.ascii_value == '\n') {
		set_enable(false);
		decision_made = false;
		return choice;
	}

	decision_made = false;
	decision_keystroke.clear();

	// infinite wait
	if (timeout_ms < 0) {
		unique_lock<mutex> l(menu_lock);
		while (decision_keystroke.ascii_value != '\n') {
			menu_cond.wait(l);
		}
	
	// wait for a specified amount of time
	} else {
		unique_lock<mutex> l(menu_lock);
		auto now = chrono::system_clock::now();
		while (menu_cond.wait_until(l, now+chrono::milliseconds(timeout_ms)) != cv_status::timeout) {
			if (decision_keystroke.ascii_value == '\n') break;
		}
	}

	// if a choice is made, disable input
	if (decision_made && decision_keystroke.ascii_value == '\n') {
		set_enable(false);
		decision_made = false;
		return choice;
	} else {
		return -1;
	}
}

// waits until a keystroke (other than navigation key) is pressed
// timeout same as wait_for_decision()
pair<int, keystroke> input_menu::wait_for_key(int timeout_ms) {

	pair<int, keystroke> result;

	if (!is_enabled()) set_enable(true);

	if (decision_made) {
		set_enable(false);
		decision_made = false;
		result.first = choice;
		result.second = decision_keystroke;
		return result;
	}

	if (timeout_ms < 0) {
		unique_lock<mutex> l(menu_lock);
		menu_cond.wait(l);
	} else {
		unique_lock<mutex> l(menu_lock);
		auto now = chrono::system_clock::now();
		menu_cond.wait_until(l, now+chrono::milliseconds(timeout_ms));
	}

	if (decision_made) {
		set_enable(false);
		decision_made = false;

		result.first = choice;
		result.second = decision_keystroke;
	} else {
		result.first = -1;
	}

	return result;

}


// commits just the attributes since the menu is lazily drawn
void input_menu::commit_all() {
	commit_attributes();
}

// redraws the menu
void input_menu::render() {

	if (!needs_redraw) {
		return;
	} else {
		lock_guard<mutex> g(pending_lock);
		
		// print the title
		bool focused = has_focus;
		set_color_unsafe(focused ? focus_color : blur_color);
		clear_pad_unsafe();
		printfc_unsafe(focused ? focus_color : blur_color, vec2d(1,1), description.c_str());

		// display menu options
		uint32_t total_options = options.size();
		int displayable_lines = pending_state.dimensions.y - desc_lines - 2; // additional 2 for border
		if (displayable_lines > 0 && total_options > 0) {

			// if top of view range has never been set, put it all the way at the top
			if (view_range_top == -1) view_range_top = 0;

			// if choice has never been set, default it to 0. otherwise if choice exceeds options size, clamp it
			if (choice == -1) {
				choice = 0;
			} else if (choice >= (int) options.size()) {
				choice = options.size()-1;
			}

			// setup the view range top
			if (choice - view_range_top < 0) {
				// need to scroll upwards, so set view range top to be the choice for the choice
				// to appear right at the top
				view_range_top = choice;
			} else if (choice - view_range_top < displayable_lines) {
				// no need to scroll -- choice is solidly within the displayable region
			} else {
				// need to scroll downards, so set the view range to be the highest possible with
				// the choice appearing at the bottom
				view_range_top = choice - displayable_lines + 1;
			}

			// print each option
			for (int counter = view_range_top; (counter < view_range_top+displayable_lines) && (counter < (int)total_options) ; ++counter) {

				// print non-highlighted option
				if (choice != (int) counter) {
					printfc_unsafe(focused ? focus_color : blur_color, vec2d(1, 1+desc_lines+counter-view_range_top), "%s", options[counter].c_str());

				// print highlighted option
				} else {
					if (focused) {
						int padding_required = pending_state.dimensions.x-2-strlen(options[counter].c_str());
						printfc_unsafe(option_color, vec2d(1, 1+desc_lines+counter-view_range_top), "%s", options[counter].c_str());
						if (padding_required > 0) {
							char buf[padding_required+1];
							memset(buf, ' ', padding_required);
							buf[padding_required] = 0;
							printfc_unsafe(option_color, "%s", buf);
						}
					} else {
						printf_unsafe(vec2d(1,1+desc_lines+counter-view_range_top), "%s", options[counter].c_str());
					}
				}
			}

			// print the scrollbar if under focus AND if there are more options than displayable lines
			if (focused && (int)total_options > displayable_lines) {
				int scrollable_lines = total_options - displayable_lines;
				float fraction_scrolled = ((float)(view_range_top) / (float)scrollable_lines);
				int scrollbar_row = fraction_scrolled * (displayable_lines-1);

				for (int counter2 = 0; counter2 < displayable_lines; ++counter2) {
					printfc_unsafe(counter2 == scrollbar_row ? scrollbar_position_color : scrollbar_color, vec2d(pending_state.dimensions.x-2, desc_lines+1+counter2), " ");
				}
			}
		}

		// clear the redraw flag and commit the output in a thread safe manner
		needs_redraw = false;
		lock_guard<mutex> g2(active_lock);
		commit_output_unsafe();
	}
}

// processes a key from the controller
void input_menu::process_keypress(const keystroke& key) {

	auto ascii_value = key.ascii_value;
	auto special_value = key.special_key;

	shared_ptr<gui_controller> controller = input_controller.lock();

	// user wants to toggle between input components. allow this if not modal
	if (ascii_value == '\t' && !modal) {
		controller->relinquish_focus(shared_from_this());

	// sanity check otherwise. nothing to do if there are no options to pick from
	} else {
	
		lock_guard<mutex> g(pending_lock);
		if (options.empty()) return;
	}

	// move up
	if (ascii_value == 0 && special_value == special_key_t::ARROW_UP) {

		{
			lock_guard<mutex> g(pending_lock);
			set_current_selection_unsafe(choice-1);
		}
		controller->refresh();

	// move down
	} else if (ascii_value == 0 && special_value == special_key_t::ARROW_DOWN) {
		{
			lock_guard<mutex> g(pending_lock);
			set_current_selection_unsafe(choice+1);
		}
		controller->refresh();

	// user hit a key on an option; notify any potential threads of the condition
	} else {

		// ignore key processing if previous key was not processed
		if (decision_made) return;

		decision_made = true;
		decision_keystroke = key;

		unique_lock<mutex> lock(menu_lock);
		menu_cond.notify_all();
	}
}

// callback when the menu receives focus
void input_menu::on_focus() {
	lock_guard<mutex> g(pending_lock);
	has_focus = true;
	needs_redraw = true;
}

// callback when the menu loses focus
void input_menu::on_blur() {
	lock_guard<mutex> g(pending_lock);
	has_focus = false;
	needs_redraw = true;
}

// sets the selection, thread unsafe
void input_menu::set_current_selection_unsafe(int choice_) {

	int old_choice = choice;

	// clamp to valid range
	if (options.empty()) {
		choice = -1;
		view_range_top = -1;
	} else if (choice_ < 0) {
		choice = 0;
	} else if (choice >= (int) options.size()) {
		choice = options.size()-1;
	} else {
		choice = choice_;
	}

	// set the redraw flag as needed
	if (choice != old_choice) {
		needs_redraw = true;
	}
}

// handles loading work
bool input_menu::load_parser_unsafe(const map<string, string>& kv_store) {

	// do low-level loading work first
	if (!gui_component::load_parser_unsafe(kv_store)) {
		return false;
	}

	// do input menu specific loading work here
	map<string, string>::const_iterator iterator;

	iterator = kv_store.find("menu_color_focus");
	if (iterator != kv_store.end()) {
		color_t color = colors::parse(iterator->second);
		set_menu_color(color);
	}

	iterator = kv_store.find("option_highlight_color");
	if (iterator != kv_store.end()) {
		color_t color = colors::parse(iterator->second);
		set_option_highlight_color(color);
	}

	iterator = kv_store.find("scrollbar_bar_color");
	if (iterator != kv_store.end()) {
		color_t color = colors::parse(iterator->second);
		set_scrollbar_color(

	}
	
}
