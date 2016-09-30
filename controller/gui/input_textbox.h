#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include "gui_component.h"
using namespace std;

namespace gui {
class input_textbox : public gui_component, public input_component {
public:

	// constructor and destructor
	input_textbox();
	virtual ~input_textbox();

	// input mode options
	void         set_modal(bool modal_status);
	bool         is_modal() const;

	// setup string to be displayed above the line input
	void         set_description(const string& desc, uint32_t max_lines);

	// setup string to be displayed to the left of the line input
	void         set_prefix(const string& prefix);

	// setup buffer
	void         set_buffer_size(uint32_t size);

	// set the focus/blur colors
	void         set_input_box_color(const color_t& focus_color, const color_t& blur_color);

	// blocks until user types in an input
	// the bool return value indicates if enter was hit (meaning the input was sent)
	// the string return value indicates the input that was typed in
	pair<bool, string> wait_for_input(int timeout_ms=-1);

	// overriden from gui component class
	virtual void commit_all();
	virtual void render();

	// overriden from the input component class
	virtual void process_keypress(const keystroke& key);
	virtual void on_focus();
	virtual void on_blur();

private:

	// disallow copies
	input_textbox(const input_textbox& other);
	input_textbox& operator=(const input_textbox& other);

	// state
	mutable mutex              box_lock;
	mutable condition_variable box_cond;
	atomic_bool                modal;
	bool                       has_focus;
	color_t                    focus_color;
	color_t                    blur_color;
	string                     description;
	string                     prefix;
	uint32_t                   desc_lines;
	uint32_t                   input_buf_size;
	char*                      input_buf;
	bool                       text_entered;
	uint32_t                   write_offset;
};

};
