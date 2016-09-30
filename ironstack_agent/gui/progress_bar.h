#pragma once
#include "gui_component.h"
using namespace std;

namespace gui {

// describes a progress bar
class progress_bar : public gui_component {
public:

	// class to define the kind of label to attach at the end of the progress bar
	enum class view_type { NONE, PERCENTAGE, NUMERICAL };

	// constructor and destructor
	progress_bar();
	~progress_bar();

	// progress bar settings
	void      set_max(int max);
	void      set_value(int val);
	void      set_view_type(const view_type& type);
	void      set_progress_bar_color(const color_t& filled_color, const color_t& empty_color);
	
	// retrieve progress bar settings
	int       get_max() const;
	int       get_value() const;
	view_type get_view_type() const;

	// override the commit_all() function so users don't have to figure that this
	// object is lazily drawn
	virtual void commit_all();

	// draws the progress bar
	virtual void render();

private:

	int       max;
	int       value;
	view_type view;
	color_t   filled_color;
	color_t   empty_color;

};

};
