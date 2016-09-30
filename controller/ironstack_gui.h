#pragma once
#include <string>
#include <thread>
#include "hal/hal.h"
#include "gui/gui_controller.h"
#include "gui/input_menu.h"
#include "gui/input_textbox.h"
#include "gui/progress_bar.h"
#include "../common/timer.h"
using namespace std;

class ironstack_gui {
public:
	ironstack_gui(const shared_ptr<hal>& controller);

	bool init();
	void shutdown();

	// thread to handle gui work
	void gui_entrypoint();

	void do_summary_screen();
	void do_flow_screen();
	void do_port_screen();
	void do_arp_screen();
	void do_cam_screen();
	void do_switch_control_screen();
	void do_experimental_screen();

	// hides all components, but doesn't commit
	void hide_summary_screen();
	void hide_flow_screen();
	void hide_port_screen();
	void hide_arp_screen();
	void hide_cam_screen();
	void hide_switch_control_screen();
	void hide_experimental_screen();

private:

	// threads
	thread gui_tid;
	thread ping_tid;

	// pointers to external objects
	shared_ptr<gui::gui_controller> display;
	shared_ptr<hal> controller;

	// global background screen
	shared_ptr<gui_component> background;
	shared_ptr<input_menu>    menu;

	// summary screen components
	shared_ptr<gui_component> summary;
	shared_ptr<gui_component> console;
	shared_ptr<progress_bar>  cpu_utilization;
	shared_ptr<progress_bar>  l2_utilization;
	shared_ptr<progress_bar>  acl_utilization;
	timer uptime_timer;

	// flow summary components
	shared_ptr<gui_component> flow_background;
	shared_ptr<input_menu>    flow_menu;

	// port summary components
	shared_ptr<gui_component> port_background;
	shared_ptr<input_menu>    port_menu;

	// arp table components
	shared_ptr<gui_component> arp_background;
	shared_ptr<input_menu>    arp_menu;

	// cam table components
	shared_ptr<gui_component> cam_background;
	shared_ptr<input_menu>    cam_menu;

	// experimental components
	shared_ptr<gui_component> experimental_background;
	shared_ptr<input_menu>		experimental_menu;
	shared_ptr<gui_component> experimental_rule_instructions;
	shared_ptr<input_textbox> experimental_textbox;

	// ping-related state
	mutex ping_lock;
	vector<pair<string, int>> pings;

	// helper functions
	void print_parenthesis(gui_component& component, const string& str, uint32_t spaces, color_t color=FG_WHITE);
	
	// ping thread entrypoint
	void ping_entrypoint();

};
