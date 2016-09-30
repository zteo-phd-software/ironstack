#include "../common/autobuf.h"
#include "gui/gui_controller.h"

/*
 * ironstack agent. this software manages low-level ironstack instances.
 *
 */



// global gui component definitions
shared_ptr<gui::gui_controller> display;
shared_ptr<gui_component> background;
shared_ptr<gui_component> instance_menu;

// executive entrypoint
int main(int argc, char** argv) {

	// init main display
	display = make_shared<gui::gui_controller>();
	display->init();
	vec2d screen_dimensions = display->get_screen_resolution();

	// setup background
	background = make_shared<gui_component>();
	background->load("display_config/background.dat");
	background->set_color(FG_WHITE | BG_BLACK);
	background->clear();
	background->printf("IronStack low-level controller management by Z. Teo (zteo@cs.cornell.edu). emer. tel: (697) 279-8025\n");
	for (int counter = 0; counter < screen_dimensions.x; ++counter) {
		background->printf("-");
	}

	// setup menu for the instance listing
	instance_menu = make_shared<gui_component>();
	instance_menu->load("display_config/instance_menu.dat");
	instance_menu->


}
