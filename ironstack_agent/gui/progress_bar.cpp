#include <memory.h>
#include <string.h>
#include "progress_bar.h"
using namespace gui;

// constructor
progress_bar::progress_bar() {
	max = 100;
	value = 0;
	view = view_type::PERCENTAGE;
	filled_color = FG_YELLOW | BG_BLUE;
	empty_color = FG_WHITE | BG_BLUE;
}

// destructor
progress_bar::~progress_bar() {}

// sets the maximum value for the progress bar
void progress_bar::set_max(int max_) {
	lock_guard<mutex> g(pending_lock);
	if (max != max_) {
		max = max_;
		needs_redraw = true;
	}
}

// sets the current value of the progress bar
void progress_bar::set_value(int val) {
	lock_guard<mutex> g(pending_lock);
	if (value != val) {
		value = val;
		needs_redraw = true;
	}
}

// sets the view type of the progress bar
void progress_bar::set_view_type(const view_type& type) {
	lock_guard<mutex> g(pending_lock);
	if (view != type) {
		view = type;
		needs_redraw = true;
	}
}

// sets the progress bar color
void progress_bar::set_progress_bar_color(const color_t& filled_color_, const color_t& empty_color_) {
	lock_guard<mutex> g(pending_lock);
	if (filled_color != filled_color_ || empty_color != empty_color_) {
		filled_color = filled_color_;
		empty_color = empty_color_;
		needs_redraw = true;
	}
}

// retrieves the max value of the progress bar
int progress_bar::get_max() const {
	lock_guard<mutex> g(pending_lock);
	return max;
}

// retrieves the current value of the progress bar
int progress_bar::get_value() const {
	lock_guard<mutex> g(pending_lock);
	return value;
}

// retrieves the current view type of the progress bar
progress_bar::view_type progress_bar::get_view_type() const {
	lock_guard<mutex> g(pending_lock);
	return view;
}

// override the commit_all() function so that users don't have to figure out that
// the object is lazily drawn
void progress_bar::commit_all() {
	commit_attributes();
}

// draws the progress bar
void progress_bar::render() {

	// don't spend CPU cycles redrawing if not needed
	if (!needs_redraw) {
		return;
	} else {
		float frac=0;
		lock_guard<mutex> g(pending_lock);
		clear_pad_unsafe();

		if (max != 0) {
			frac = (float)value /(float)max;
		}

		// construct label
		char buf[128];
		switch (view) {
		case view_type::NONE:
			buf[0] = 0;
			break;
		case view_type::PERCENTAGE:
			if (max != 0) {
				sprintf(buf, "%6.2f%%", frac*100.0);
			} else {
				sprintf(buf, "inf.");
			}
			break;
		case view_type::NUMERICAL:
			sprintf(buf, "%3d/%3d", value, max);
			break;
		}

		// subtract extra 2 space to create gap between bar/label/end
		int label_len = strlen(buf);
		int cols_available = label_len > 0 ? (pending_state.dimensions.x - strlen(buf) - 2)
			: pending_state.dimensions.x-1;
		
		// clamp the frac value before converting it into numbers for shading
		float clamped_frac = frac;
		if (clamped_frac <= 0.0f) {
			clamped_frac = 0.0f;
		} else if (clamped_frac > 1.0f) {
			clamped_frac = 1.0f;
		}

		int cols_to_shade = clamped_frac * cols_available;
		int cols_to_unshade = cols_available - cols_to_shade;
		if (cols_available < 0 || cols_to_shade < 0 || cols_to_unshade < 0) return;

		// construct the space array for the filled portion
		char buf2[cols_to_shade+1];
		memset(buf2, ' ', cols_to_shade);
		buf2[cols_to_shade] = 0;
		printfc_unsafe(filled_color, vec2d(0,0), "%s", buf2);

		// construct the space array for the empty portion
		if (cols_to_unshade > 1) {		
			char buf3[cols_to_unshade+1];
			if (cols_to_unshade > 0) {
				memset(buf3, ' ', cols_to_unshade);
			}
			buf3[cols_to_unshade] = 0;
			printfc_unsafe(empty_color, "%s", buf3);
		}

		// display the label, if any.
		if (label_len > 0) {
			printfc_unsafe(pending_draw_state.current_color, " %s", buf);
		}
	
		// acquire remaining lock and commit the output in a thread-safe manner
		needs_redraw = false;
		lock_guard<mutex> g2(active_lock);
		commit_output_unsafe();
	}
}
