#include <memory.h>
#include <string.h>
#include "gui_controller.h"
#include "input_textbox.h"
using namespace gui;

// constructor
input_textbox::input_textbox() {
	modal = false;
	has_focus = false;
	focus_color = FG_WHITE | BG_BLUE;
	blur_color = FG_GRAY | BG_BLACK;
	desc_lines = 0;
	input_buf_size = 128;
	input_buf = (char*) malloc(input_buf_size);
	if (input_buf == nullptr) {
		abort();
	}
	text_entered = false;
	write_offset = 0;
}

// destructor
input_textbox::~input_textbox() {
	if (input_buf != nullptr) {
		free(input_buf);
	}
}

// sets the modality of the textbox
void input_textbox::set_modal(bool modality) {
	lock_guard<mutex> g(pending_lock);
	modal = modality;
}

// checks if the textbox is in modal mode
bool input_textbox::is_modal() const {
	lock_guard<mutex> g(pending_lock);
	return modal;
}

// sets the description to appear in the line above the input
void input_textbox::set_description(const string& desc, uint32_t max_lines) {
	lock_guard<mutex> g(pending_lock);
	if (description != desc || desc_lines != max_lines) {
		description = desc;
		desc_lines = max_lines;
		needs_redraw = true;
	}
}

// set the prefix to appear before every line of input
void input_textbox::set_prefix(const string& prefix_) {
	lock_guard<mutex> g(pending_lock);
	if (prefix != prefix_) {
		prefix = prefix_;
		needs_redraw = true;
	}
}

// sets up the input buffer size
void input_textbox::set_buffer_size(uint32_t size) {
	lock_guard<mutex> g(pending_lock);
	input_buf_size = size;
	if (size == 0) {
		free(input_buf);
		input_buf = nullptr;
	} else {
		input_buf = (char*) realloc(input_buf, size);
		if (input_buf == nullptr) {
			abort();
		} else {
			memset(input_buf, 0, size);
		}
	}
}

// set up colors for the texbox
void input_textbox::set_input_box_color(const color_t& focus_color_, const color_t& blur_color_) {
	lock_guard<mutex> g(pending_lock);
	if (focus_color != focus_color_ || blur_color != blur_color_) {
		focus_color = focus_color_;
		blur_color = blur_color_;
		needs_redraw = true;
	}
}

// block until user types in an input
pair<bool, string> input_textbox::wait_for_input(int timeout_ms) {

	pair<bool, string> result;

	if (!is_enabled()) set_enable(true);

	if (text_entered) {
		set_enable(false);
		text_entered = false;

		result.first = true;
		result.second = input_buf;
		return result;
	}

	if (timeout_ms < 0) {
		unique_lock<mutex> g(box_lock);
		box_cond.wait(g);

		result.first = true;
		result.second = input_buf;
		input_buf[0] = 0;
		write_offset = 0;

	} else {
		unique_lock<mutex> g(box_lock);
		auto now = chrono::system_clock::now();
		while (box_cond.wait_until(g, now+chrono::milliseconds(timeout_ms)) != cv_status::timeout) {
			if (text_entered) {
				result.first = true;
				result.second = input_buf;
				input_buf[0] = 0;
				write_offset = 0;
				break;
			}
		}
	}

	if (text_entered) {
		text_entered = false;
		set_enable(false);
	} else {
		result.first = false;
	}

	return result;
}

// commits just the attributes since this object is lazily drawn
void input_textbox::commit_all() {
	commit_attributes();
}

// renders the textbox
void input_textbox::render() {

	if (!needs_redraw) {
		return; 
	} else {
		lock_guard<mutex> g(pending_lock);
		bool focused = has_focus;

		set_color_unsafe(focused ? focus_color : blur_color);
		clear_pad_unsafe();

		// write the user input first
		printfc_unsafe(focused ? focus_color : blur_color, vec2d(0, desc_lines), "%s", prefix.c_str());
		if (input_buf != nullptr) {
			printfc_unsafe(focused ? focus_color : blur_color, "%s", input_buf);
		}
		auto old_pos = pending_draw_state.cursor_position;

		// go back to the top row and display the description
		char space_buf[pending_state.dimensions.x+1];
		memset(space_buf, ' ', pending_state.dimensions.x);
		space_buf[pending_state.dimensions.x] = 0;
		for (int counter = 0; counter < (int) desc_lines; ++counter) {
			printfc_unsafe(focused ? focus_color : blur_color, vec2d(0,counter), "%s", space_buf);
		}
		printfc_unsafe(focused ? focus_color : blur_color, vec2d(0,0), "%s", description.c_str());

		if (focused) {
			set_cursor_unsafe(old_pos);
			set_cursor_visible_unsafe(true);
		} else {
			set_cursor_visible_unsafe(false);
		}

		// commit to output
		needs_redraw = false;
		lock_guard<mutex> g2(active_lock);
		commit_output_unsafe();
	}
}

// callback to handle keys
void input_textbox::process_keypress(const keystroke& key) {

	auto ascii_value = key.ascii_value;

	shared_ptr<gui_controller> controller = input_controller.lock();

	// switching between input components
	if (ascii_value == '\t' && !modal) {
		controller->relinquish_focus(shared_from_this());

	// store printable characters
	} else if (ascii_value >= 32 && ascii_value <= 126) {

		bool written = false;
		{
			lock_guard<mutex> g(pending_lock);
			if (write_offset < input_buf_size) {
				input_buf[write_offset] = ascii_value;
				input_buf[++write_offset] = 0;
				written = true;
				needs_redraw = true;
			}
		}
		if (written) {
			render();
			controller->refresh();
		}

	// handle backspace
	} else if (ascii_value == '\b') {
		bool written = false;
		{
			lock_guard<mutex> g(pending_lock);
			if (write_offset > 0) {
				input_buf[--write_offset] = 0;
				written = true;
				needs_redraw = true;
			}
		}
		if (written) {
			render();
			controller->refresh();
		}

	// user finished with input
	} else if (ascii_value == '\n') {
		set_enable(false);
		text_entered = true;
		set_cursor_visible(false);
		unique_lock<mutex> lock(box_lock);
		box_cond.notify_all();
	}
}

// callback when the input box receives focus
void input_textbox::on_focus() {
	lock_guard<mutex> g(pending_lock);
	has_focus = true;
	needs_redraw = true;
}

// callback when the input box loses focus
void input_textbox::on_blur() {
	lock_guard<mutex> g(pending_lock);
	has_focus = false;
	needs_redraw = true;
}
