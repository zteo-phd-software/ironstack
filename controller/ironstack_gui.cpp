#include "ironstack_gui.h"
#include "gui/output.h"
#include "services/arp.h"
#include "services/cam.h"
#include "services/dell_s48xx_acl_table.h"
#include "services/dell_s48xx_l2_table.h"
#include "services/flow_service.h"
#include "services/ironstack_echo_daemon.h"
#include "services/operational_stats.h"
#include "utils/ethernet_mac_db.h"
#include "../common/common_utils.h"

extern shared_ptr<dell_s48xx_l2_table> l2_table;
extern shared_ptr<dell_s48xx_acl_table> acl_table;
extern uint64_t controller_bytes_sent;
extern uint64_t controller_bytes_received;
extern string switch_name;
extern ethernet_mac_db dl_address_db;
extern bool experimental_auto_broadcast_rules;
extern bool experimental_auto_broadcast_rules_installed;

// constructor takes in a pointer to the hal
ironstack_gui::ironstack_gui(const shared_ptr<hal>& controller_) {
	controller = controller_;
}

// initializes display
bool ironstack_gui::init() {
	display = make_shared<gui::gui_controller>();
	display->init();
	
	// setup background
	vec2d screen_dimensions = display->get_screen_resolution();
	background = make_shared<gui_component>();
	background->set_origin(vec2d(0,0));
	background->set_dimensions(screen_dimensions);
	background->set_z_position(1000);
	background->set_cursor_visible(false);
	background->set_color(FG_WHITE | BG_BLACK);
	background->clear();
	background->printf("IronStack OpenFlow controller v0.4b by Z. Teo (zteo@cs.cornell.edu). emer. tel: (607) 279-8025\n");
	for (int counter = 0; counter < screen_dimensions.x; ++counter) {
		background->printf("-");
	}
	background->printf(vec2d(2,41), "console output");
	background->commit_all();
	display->register_gui_component(background);

	// setup console output
	console = make_shared<gui_component>();
	console->set_origin(vec2d(2, 42));
	console->set_z_position(1);
	console->set_cursor_visible(true);
	console->set_cursor_color(FG_WHITE | BG_YELLOW);
	console->set_dimensions(vec2d(120, 15));
	console->set_color(FG_WHITE | BG_BLUE);
	console->clear();
	console->set_visible(true);
	console->commit_all();
	display->register_gui_component(console);
	display->associate_thread_with_component(thread::id(), console);	// all threads default output to this component

	/* components for operational summary stats */
	// for summary of operational stats (main screen)
	summary = make_shared<gui_component>();
	summary->set_origin(vec2d(0, 2));
	summary->set_dimensions(vec2d(screen_dimensions.x, 39));
	summary->set_z_position(999);
	summary->set_cursor_visible(false);
	summary->set_color(FG_WHITE | BG_BLACK);
	summary->clear();
	summary->set_visible(false);
	summary->commit_all();
	display->register_gui_component(summary);

	// progress bar displaying CPU usage
	cpu_utilization = make_shared<progress_bar>();
	cpu_utilization->set_origin(vec2d(21, 27));
	cpu_utilization->set_dimensions(vec2d(32,1));
	cpu_utilization->set_z_position(997);
	cpu_utilization->set_progress_bar_color(FG_WHITE | BG_YELLOW, FG_WHITE | BG_BLUE);
	cpu_utilization->set_view_type(gui::progress_bar::view_type::PERCENTAGE);
	cpu_utilization->set_value(0);
	cpu_utilization->set_visible(false);
	cpu_utilization->commit_all();
	display->register_gui_component(cpu_utilization);

	// progress bar displaying L2 table usage
	l2_utilization = make_shared<progress_bar>();
	l2_utilization->set_origin(vec2d(21,28));
	l2_utilization->set_dimensions(vec2d(32,1));
	l2_utilization->set_z_position(997);
	l2_utilization->set_progress_bar_color(FG_WHITE | BG_YELLOW, FG_WHITE | BG_BLUE);
	l2_utilization->set_view_type(gui::progress_bar::view_type::NUMERICAL);
	l2_utilization->set_max(l2_table->get_max_capacity());
	l2_utilization->set_value(0);
	l2_utilization->set_visible(false);
	l2_utilization->commit_all();
	display->register_gui_component(l2_utilization);

	// progress bar displaying ACL table usage
	acl_utilization = make_shared<progress_bar>();
	acl_utilization->set_origin(vec2d(21,29));
	acl_utilization->set_dimensions(vec2d(32,1));
	acl_utilization->set_z_position(997);
	acl_utilization->set_progress_bar_color(FG_WHITE | BG_YELLOW, FG_WHITE | BG_BLUE);
	acl_utilization->set_view_type(gui::progress_bar::view_type::NUMERICAL);
	acl_utilization->set_max(acl_table->get_max_capacity());
	acl_utilization->set_value(0);
	acl_utilization->set_visible(false);
	acl_utilization->commit_all();
	display->register_gui_component(acl_utilization);

	/* components for flow summary stats */
	// background for the flow information
	flow_background = make_shared<gui_component>();
	flow_background->set_origin(vec2d(0, 2));
	flow_background->set_dimensions(vec2d(screen_dimensions.x, 39));
	flow_background->set_z_position(999);
	flow_background->set_cursor_visible(false);
	flow_background->set_color(FG_WHITE | BG_BLACK);
	flow_background->clear();
	flow_background->set_visible(false);
	flow_background->printf(vec2d(0,0), "%s flow summary", switch_name.substr(10).c_str());
	flow_background->printfc(FG_CYAN | BG_BLACK, vec2d(2,2), "id");
	flow_background->printfc(FG_CYAN | BG_BLACK, vec2d(7,2), "match criteria");
	flow_background->printfc(FG_CYAN | BG_BLACK, vec2d(50,2), "actions");
	flow_background->printfc(FG_CYAN | BG_BLACK, vec2d(65,2), "alive");
	flow_background->printfc(FG_CYAN | BG_BLACK, vec2d(86,2), "install reason");
	flow_background->commit_all();
	display->register_gui_component(flow_background);

	// actual menu that houses the flow listing
	flow_menu = make_shared<input_menu>();
	flow_menu->set_origin(vec2d(0,5));
	flow_menu->set_z_position(998);
	flow_menu->set_dimensions(vec2d(120,30));
	flow_menu->set_menu_color(FG_WHITE | BG_BLACK, FG_WHITE | BG_BLACK);
	flow_menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	flow_menu->set_scrollbar_color(FG_WHITE | BG_CYAN, FG_CYAN | BG_WHITE);
	flow_menu->set_visible(false);
	flow_menu->set_enable(false);
	flow_menu->commit_all();
	display->register_gui_component(flow_menu);
	display->register_input_component(flow_menu);

	/* components for port summary stats */
	port_background = make_shared<gui_component>();
	port_background->set_origin(vec2d(0, 2));
	port_background->set_dimensions(vec2d(screen_dimensions.x, 39));
	port_background->set_z_position(999);
	port_background->set_cursor_visible(false);
	port_background->set_color(FG_WHITE | BG_BLACK);
	port_background->clear();
	port_background->set_visible(false);
	port_background->printf(vec2d(0,0), "%s switch port summary", switch_name.substr(10).c_str());
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(2,2), "port");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(10,2), "type");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(20,2), "status");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(28,2), "vlans");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(54,2), "tx pkts");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(64,2), "tx bytes");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(74,2), "rx pkts");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(84,2), "rx bytes");
	port_background->printfc(FG_CYAN | BG_BLACK, vec2d(94,2), "remarks");
	port_background->commit_all();
	display->register_gui_component(port_background);

	// actual menu that houses the port listing
	port_menu = make_shared<input_menu>();
	port_menu->set_origin(vec2d(0,5));
	port_menu->set_z_position(998);
	port_menu->set_dimensions(vec2d(120,30));
	port_menu->set_menu_color(FG_WHITE | BG_BLACK, FG_WHITE | BG_BLACK);
	port_menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	port_menu->set_scrollbar_color(FG_WHITE | BG_CYAN, FG_CYAN | BG_WHITE);
	port_menu->set_visible(false);
	port_menu->set_enable(false);
	port_menu->commit_all();
	display->register_gui_component(port_menu);
	display->register_input_component(port_menu);

	/* components for ARP table */
	// the background that contains the header labels
	arp_background = make_shared<gui_component>();
	arp_background->set_origin(vec2d(0, 2));
	arp_background->set_dimensions(vec2d(screen_dimensions.x, 39));
	arp_background->set_z_position(999);
	arp_background->set_cursor_visible(false);
	arp_background->set_color(FG_WHITE | BG_BLACK);
	arp_background->clear();
	arp_background->set_visible(false);
	arp_background->printf(vec2d(0,0), "%s ARP table", switch_name.substr(10).c_str());
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(2,2), "vlan");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(8,2), "IP address");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(25,2), "MAC address");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(44,2), "vendor");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(70,2), "lookup count");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(85,2), "last lookup");
	arp_background->printfc(FG_CYAN | BG_BLACK, vec2d(99,2), "last updated");
	arp_background->commit_all();
	display->register_gui_component(arp_background);

	// actual menu that houses the port listing
	arp_menu = make_shared<input_menu>();
	arp_menu->set_origin(vec2d(0,5));
	arp_menu->set_z_position(998);
	arp_menu->set_dimensions(vec2d(120,30));
	arp_menu->set_menu_color(FG_WHITE | BG_BLACK, FG_WHITE | BG_BLACK);
	arp_menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	arp_menu->set_scrollbar_color(FG_WHITE | BG_CYAN, FG_CYAN | BG_WHITE);
	arp_menu->set_visible(false);
	arp_menu->set_enable(false);
	arp_menu->commit_all();
	display->register_gui_component(arp_menu);
	display->register_input_component(arp_menu);

	/* components for CAM table */
	// the background that contains the header labels
	cam_background = make_shared<gui_component>();
	cam_background->set_origin(vec2d(0, 2));
	cam_background->set_dimensions(vec2d(screen_dimensions.x, 39));
	cam_background->set_z_position(999);
	cam_background->set_cursor_visible(false);
	cam_background->set_color(FG_WHITE | BG_BLACK);
	cam_background->clear();
	cam_background->set_visible(false);
	cam_background->printf(vec2d(0,0), "%s CAM table", switch_name.substr(10).c_str());
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(2,2), "vlan");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(8,2), "MAC address");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(27,2), "vendor");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(53,2), "port");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(61,2), "lookup count");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(76,2), "last lookup");
	cam_background->printfc(FG_CYAN | BG_BLACK, vec2d(90,2), "last updated");
	cam_background->commit_all();
	display->register_gui_component(cam_background);

	// actual menu that houses the port listing
	cam_menu = make_shared<input_menu>();
	cam_menu->set_origin(vec2d(0,5));
	cam_menu->set_z_position(998);
	cam_menu->set_dimensions(vec2d(120,30));
	cam_menu->set_menu_color(FG_WHITE | BG_BLACK, FG_WHITE | BG_BLACK);
	cam_menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	cam_menu->set_scrollbar_color(FG_WHITE | BG_CYAN, FG_CYAN | BG_WHITE);
	cam_menu->set_visible(false);
	cam_menu->set_enable(false);
	cam_menu->commit_all();
	display->register_gui_component(cam_menu);
	display->register_input_component(cam_menu);
	
	// menu that appears on the RHS of the screen
	menu = make_shared<input_menu>();
	menu->set_origin(vec2d(103,10));
	menu->set_z_position(0);
	menu->set_dimensions(vec2d(20,10));
	menu->set_menu_color(FG_WHITE | BG_BLUE, FG_WHITE | BG_BLACK);
	menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	menu->set_description("select screen:",1);
	menu->add_option("summary stats");
	menu->add_option("flow table");
	menu->add_option("switch ports");
	menu->add_option("arp table");
	menu->add_option("cam table");
	menu->add_option("switch control");
	menu->add_option("experimental");
	menu->set_visible(false);
	menu->commit_all();
	display->register_gui_component(menu);
	display->register_input_component(menu);

	/* components for CAM table */
	// the background that contains the header labels
	experimental_background = make_shared<gui_component>();
	experimental_background->set_origin(vec2d(0, 2));
	experimental_background->set_dimensions(vec2d(screen_dimensions.x, 39));
	experimental_background->set_z_position(999);
	experimental_background->set_cursor_visible(false);
	experimental_background->set_color(FG_WHITE | BG_BLACK);
	experimental_background->clear();
	experimental_background->set_visible(false);
	experimental_background->printf(vec2d(0,0), "%s experimental features", switch_name.substr(10).c_str());
	experimental_background->printfc(FG_CYAN | BG_BLACK, vec2d(2,2), "hardware broadcast rules");
	experimental_background->printfc(FG_CYAN | BG_BLACK, vec2d(45,2), "manual rule addition");
	experimental_background->commit_all();
	display->register_gui_component(experimental_background);

	// menu that appears on the RHS of the screen
	experimental_menu = make_shared<input_menu>();
	experimental_menu->set_origin(vec2d(2, 6));
	experimental_menu->set_z_position(998);
	experimental_menu->set_dimensions(vec2d(28,6));
	experimental_menu->set_menu_color(FG_WHITE | BG_BLUE, FG_WHITE | BG_BLACK);
	experimental_menu->set_option_highlight_color(FG_BLACK | BG_GREEN);
	experimental_menu->add_option("enable");
	experimental_menu->add_option("disable");
	experimental_menu->set_visible(false);
	experimental_menu->set_enable(false);
	experimental_menu->commit_all();
	display->register_gui_component(experimental_menu);
	display->register_input_component(experimental_menu);

	// text panel to show instructions
	experimental_rule_instructions = make_shared<gui_component>();
	experimental_rule_instructions->set_origin(vec2d(45,6));
	experimental_rule_instructions->set_dimensions(vec2d(53,12));
	experimental_rule_instructions->set_z_position(998);
	experimental_rule_instructions->set_cursor_visible(false);
	experimental_rule_instructions->set_color(FG_WHITE | BG_BLACK);
	experimental_rule_instructions->clear();
	experimental_rule_instructions->set_visible(false);
	experimental_rule_instructions->printf("match syntax:\n"
	         "in_port   = [n | *]\n"
	         "dl_src    = [eth addr | *]  dl_dest  = [eth addr | *]\n"
	         "vlan_id   = [n | *]         vlan_pcp = [n | *]\n"
					 "ethertype = [n | *]\n"
	         "ip_tos    = [n | *]         ip_proto = [n | *]\n"
	         "nw_src    = [ip addr]       nw_dest  = [ip addr]\n"
	         "nw_src_lsb_wildcard  = [n, 0 <= n <= 32]\n"
	         "nw_dest_lsb_wildcard = [n, 0 <= n <= 32]\n"
	         "-----------------------------------------------------");
	experimental_rule_instructions->commit_all();
	display->register_gui_component(experimental_rule_instructions);

	// input textbox for manual flow creation
	experimental_textbox = make_shared<input_textbox>();
	experimental_textbox->set_origin(vec2d(45,17));
	experimental_textbox->set_z_position(998);
	experimental_textbox->set_cursor_visible(true);
	experimental_textbox->set_dimensions(vec2d(53,3));
	experimental_textbox->set_input_box_color(FG_WHITE | BG_BLUE, FG_WHITE | BG_BLACK);
	experimental_textbox->set_buffer_size(512);
	experimental_textbox->set_prefix("> ");
	experimental_textbox->set_enable(false);
	display->register_gui_component(experimental_textbox);
	display->register_input_component(experimental_textbox);

	// redirect output to display controller
	// and setup logging
	output::init(display);
	if (switch_name.size() < 10) {
		assert(false && "ironstack_gui::init() -- switch name cannot be empty.");
		abort();
	} 
	string log_name = string("logs/") + switch_name.substr(10) + string(".log");
	output::start_log(log_name, true, output::loglevel::INFO);

	// start the gui execution code in a separate thread
	gui_tid = thread(&ironstack_gui::gui_entrypoint, this);
	ping_tid = thread(&ironstack_gui::ping_entrypoint, this);
	return true;
}

// shuts down the display
void ironstack_gui::shutdown() {
	output::shutdown();
	display = nullptr;
}

// entrypoint for GUI code after initialization
void ironstack_gui::gui_entrypoint() {

	int current_screen = 0;
	uptime_timer.start();
	
	// do the summary screen until switch state reports ready
	while(1) {
		do_summary_screen();
		shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

		if (sw_state == nullptr || !sw_state->is_switch_ready()) {
			timer::sleep_for_ms(500);
		} else {
			break;
		}
	}

	// make menu usable
	menu->set_enable(true);
	menu->set_visible(true);
	menu->commit_attributes();
	menu->set_focus();

	// switch is ready. enable menu and allow input
	while(1) {
		int choice = 0;

		menu->set_enable(true);
		menu->set_visible(true);
		menu->commit_attributes();
		choice = menu->wait_for_decision(500);	// wait 500ms for selection

		// hide the current screen if needed
		if (choice != -1) {
			switch(current_screen) {
				case 0: // summary screen
				{
					hide_summary_screen();
					break;
				}
				case 1:	// flow table
				{
					hide_flow_screen();
					break;
				}
				case 2:	// port screen
				{
					hide_port_screen();
					break;
				}
				case 3: // arp screen
				{
					hide_arp_screen();
					break;
				}
				case 4:
				{
					hide_cam_screen();
					break;
				}
				case 5:
				{
					hide_switch_control_screen();
					break;
				}
				case 6:
				{
					hide_experimental_screen();
				}
				default:
					break;
			}

			current_screen = choice;
		}

		// refresh the current screen as needed
		switch(current_screen) {
			case 0:		// summary screen
			{
				do_summary_screen();
				break;
			}

			case 1:		// flow table
			{
				do_flow_screen();
				break;
			}

			case 2:		// switch ports
			{
				do_port_screen();
				break;
			}

			case 3:		// arp table
			{
				do_arp_screen();
				break;
			}

			case 4:		// cam table
			{
				do_cam_screen();
				break;
			}

			case 5:		// switch control
			{
				do_switch_control_screen();
				break;
			}
			case 6:
			{
				do_experimental_screen();
				break;
			}
			default:
				break;
		}
	}
}

// displays the summary screen
void ironstack_gui::do_summary_screen() {

	// store some state here
//	uint64_t last_bytes_up = 0;
//	uint64_t last_bytes_down = 0;

	// grab pointers to services
	shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

	// perform refresh
	gui_component& bg = *summary;
	bg.set_visible(true);
	bg.clear();
	bg.printfc(FG_GREEN, "%s operational summary", switch_name.substr(10).c_str());

	// if switch state isn't ready, the switch hasn't connected
	if (sw_state == nullptr) {
		int sec_elapsed = uptime_timer.get_time_elapsed_ms() / 1000;

		bg.printf(vec2d(2,2), "standby while the switch connects to this controller... ");
		switch (sec_elapsed % 4) {
			case 0:
			{
				bg.printf("/");
				break;
			}
			case 1:
			{
				bg.printf("-");
				break;
			}
			case 2:
			{
				bg.printf("\\");
				break;
			}
			case 3:
			{
				bg.printf("|");
				break;
			}
			display->refresh();
		}

	// switch has connected, but information isn't available yet
	} else if (!sw_state->is_switch_ready()) {
		int sec_elapsed = uptime_timer.get_time_elapsed_ms() / 1000;

		bg.printfc(FG_YELLOW, vec2d(2,2), "switch connected; updating information... ");
		switch (sec_elapsed % 4) {
			case 0:
			{
				bg.printf("/");
				break;
			}
			case 1:
			{
				bg.printf("-");
				break;
			}
			case 2:
			{
				bg.printf("\\");
				break;
			}
			case 3:
			{
				bg.printf("|");
				break;
			}
			display->refresh();
		}

	// switch connected and state is ready. display some actual stats
	} else {

		// display switch-specific information from database
		aux_switch_info info = sw_state->get_aux_switch_info();
		bg.printfc(FG_CYAN, vec2d(2, 3), "switch information");
		bg.printf(vec2d(2,4), "switch name      : ");
		print_parenthesis(bg, info.switch_name, 30, FG_RED);
		bg.printf(vec2d(2,5), "switch type      : ");
		print_parenthesis(bg, info.switch_type, 30);
		bg.printf(vec2d(2,6), "datapath ID      : ");
		print_parenthesis(bg, info.datapath_id.to_string(), 30);
		bg.printf(vec2d(2,7), "room             : ");
		print_parenthesis(bg, info.room, 30);
		bg.printf(vec2d(2,8), "location         : ");
		print_parenthesis(bg, info.switch_location, 30);
		bg.printf(vec2d(2,9), "management IP    : ");
		print_parenthesis(bg, info.management_ip.to_string(), 30, FG_YELLOW);
		bg.printf(vec2d(2,10), "control plane IP : ");
		print_parenthesis(bg, info.control_plane3_ip.to_string(), 30, FG_YELLOW);

		// display openflow description
		openflow_switch_description description = sw_state->get_switch_description();
		ip_address switch_ip = sw_state->get_switch_ip();
		mac_address switch_mac = sw_state->get_switch_mac();

		bg.printfc(FG_CYAN, vec2d(2,12), "switch hardware information");
		bg.printf(vec2d(2,13), "hw manufacturer  : ");
		print_parenthesis(bg, description.manufacturer, 30);
		bg.printf(vec2d(2,14), "hw description   : ");
		print_parenthesis(bg, description.hardware_description, 30);
		bg.printf(vec2d(2,15), "sw description   : ");
		print_parenthesis(bg, description.software_description, 30);
		bg.printf(vec2d(2,16), "hw serial number : ");
		print_parenthesis(bg, description.serial_number, 30);
		bg.printf(vec2d(2,17), "hw general desc  : ");
		print_parenthesis(bg, description.general_description, 30);

		// display controller information
		bg.printfc(FG_CYAN, vec2d(2,19), "controller information");
		bg.printf(vec2d(2,20), "controller IP    : ");
		print_parenthesis(bg, switch_ip.to_string(), 30, FG_YELLOW);
		bg.printf(vec2d(2,21), "controller MAC   : ");
		print_parenthesis(bg, switch_mac.to_string(), 30);

		bg.printf(vec2d(2,22), "software uptime  : ");
		char buf[30];
		int sec_elapsed = uptime_timer.get_time_elapsed_ms()/1000;
		int seconds = sec_elapsed % 60;
		int minutes = (sec_elapsed / 60) % 60;
		int hours = sec_elapsed / 3600;
		sprintf(buf, "%d hours %d minutes %d seconds", hours, minutes, seconds);
		print_parenthesis(bg, buf, 30);

		// update utilization meters
		bg.printfc(FG_CYAN, vec2d(2,24), "resource utilization");
		bg.printf(vec2d(2,25), "cpu utilization  : ");
		bg.printf(vec2d(2,26), "l2 utilization   : ");
		bg.printf(vec2d(2,27), "acl utilization  : ");

		double cpu_utilization_val = cu_get_cpu_utilization();
		cpu_utilization->set_value(cpu_utilization_val);
		int total_l2 = l2_table->get_max_capacity();
		int used_l2 = total_l2 - l2_table->get_available_capacity();
		l2_utilization->set_value(used_l2);
		int total_acl = acl_table->get_max_capacity();
		int used_acl = total_acl - acl_table->get_available_capacity();
		acl_utilization->set_value(used_acl);
		cpu_utilization->set_visible(true);
		l2_utilization->set_visible(true);
		acl_utilization->set_visible(true);
		cpu_utilization->commit_all();
		l2_utilization->commit_all();
		acl_utilization->commit_all();

		// update controller up/down rate
		bg.printfc(FG_CYAN, vec2d(2, 29), "switch/controller connection load");
		bg.printf(vec2d(2,30), "bytes/rate up    : ");
//		sprintf(buf, "%15"PRIu64"/%8"PRIu64" bytes", l

		sprintf(buf, "%24" PRIu64 " bytes", controller_bytes_sent);
		print_parenthesis(bg, buf, 30);
		bg.printf(vec2d(2,31), "bytes/rate down  : ");
		sprintf(buf, "%24" PRIu64 " bytes", controller_bytes_received);
		print_parenthesis(bg, buf, 30);

		// display ping information
		vector<pair<string, int>> latencies;
		{
			lock_guard<mutex> g(ping_lock);
			latencies = pings;
		}

		uint32_t table_size = latencies.size();
		bg.printfc(FG_CYAN, vec2d(65,3), "distributed controller status");
		for (uint32_t counter = 0; counter < table_size; ++counter) {
			bg.printf(vec2d(65, 4+counter), "%s", latencies[counter].first.c_str());
			if (latencies[counter].second == -1) {
				bg.printfc(FG_RED, vec2d(85, 4+counter), "offline");
			} else if (latencies[counter].second == -2) {
				bg.printfc(FG_YELLOW, vec2d(85, 4+counter), "updating");
			} else {
				bg.printfc(FG_GREEN, vec2d(85, 4+counter), "%dms", latencies[counter].second);
			}
		}
	}

	// commit changes
	bg.commit_all();
	display->refresh();
}

// displays a list of all flows
void ironstack_gui::do_flow_screen() {

	// when was the last time this screen was entered? to prevent spamming the switch, we refresh flows at most once every 2 seconds
	static bool measurement_started = false;
	static timer measurement_timer;
	if (!measurement_started) {
		measurement_started = true;
		measurement_timer.reset();
	} else if (measurement_timer.get_time_elapsed_ms() < 2000) {
		return;
	} else {
		measurement_timer.reset();
	}

	// grab pointers to services
	shared_ptr<operational_stats> op_stats_svc = static_pointer_cast<operational_stats>(controller->get_service(service_catalog::service_type::OPERATIONAL_STATS));
	shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(controller->get_service(service_catalog::service_type::FLOWS));

	// if switch isn't online, don't do anything
	if (sw_state == nullptr || !sw_state->is_switch_ready()) {
		flow_menu->printfc(FG_RED | BG_BLACK, vec2d(1,1), "flows unavailable until controller is online.");
	} else {

		op_stats_svc->update_flow_stats();
		auto flow_stats = op_stats_svc->get_flow_stats();
		map<uint64_t, openflow_flow_entry> flow_entries = flow_svc->get_flows_map();

		// if there are no flows, there's nothing to select
		uint32_t num_flows = flow_stats.size();
		if (num_flows == 0) {
			flow_menu->clear_options();
			flow_menu->add_option("no flows active.");
			flow_menu->wait_for_key(0);	// used to discard input
		// display each flow
		} else {
			int old_selection = flow_menu->get_current_selection();
			if (old_selection < 0) old_selection = 0;
			flow_menu->clear_options();
			int counter = 1;
			for (const auto& flow : flow_stats) {

				char id_buf[32];
				sprintf(id_buf, "  %d", counter++);

				char buf[1024];
				memset(buf, ' ', sizeof(buf));
				string criteria = flow.flow_description.criteria.to_string();
				string action_list = flow.flow_description.action_list.to_string();
				char alive_buf[32];
				sprintf(alive_buf, "%dhr %dmin %dsec", flow.duration_alive_sec / 3600,
					(flow.duration_alive_sec / 60) % 60,
					flow.duration_alive_sec % 60);

				memcpy(buf, id_buf, strlen(id_buf));
				memcpy(buf+6, criteria.c_str(), criteria.size());
				memcpy(buf+49, action_list.c_str(), action_list.size());
				memcpy(buf+64, alive_buf, strlen(alive_buf));

				auto iterator = flow_entries.find(flow.flow_description.cookie);
				if (iterator != flow_entries.end()) {
					memcpy(buf+85, iterator->second.install_reason.c_str(), iterator->second.install_reason.size());
				} else {
					strcpy(buf+85, "[deleting]");
				}

				// cap display at 103 chars
				buf[104] = 0;
				flow_menu->add_option(buf);
			}
			flow_menu->set_current_selection(old_selection);

			// wait for selection
			auto result = flow_menu->wait_for_key(0);
			if (result.first != -1) {
				if (result.second.special_key == special_key_t::DELETE) {
					uint64_t cookie = flow_stats[result.first].flow_description.cookie;
					flow_svc->remove_flow(cookie);
				}
			}
		}
	}

	// commit changes
	flow_background->set_visible(true);
	flow_background->commit_attributes();
	flow_menu->set_visible(true);
	flow_menu->set_enable(true);
	flow_menu->commit_all();
	display->refresh();
}

// displays all ports associated with the controller
void ironstack_gui::do_port_screen() {

	// when was the last time this screen was entered? to prevent spamming the switch, we refresh flows at most once every 2 seconds
	static bool measurement_started = false;
	static timer measurement_timer;
	if (!measurement_started) {
		measurement_started = true;
		measurement_timer.reset();
	} else if (measurement_timer.get_time_elapsed_ms() < 2000) {
		return;
	} else {
		measurement_timer.reset();
	}

	// grab pointers to services
	shared_ptr<operational_stats> op_stats_svc = static_pointer_cast<operational_stats>(controller->get_service(service_catalog::service_type::OPERATIONAL_STATS));
	shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

	// if switch isn't online, don't do anything
	if (sw_state == nullptr || !sw_state->is_switch_ready()) {
		flow_menu->printfc(FG_RED | BG_BLACK, vec2d(1,1), "ports unavailable until controller is online.");

	// grab ports and update display
	} else {
		int old_selection = port_menu->get_current_selection();
		port_menu->clear_options();

		op_stats_svc->update_port_stats();
		vector<openflow_port_stats> port_stats = op_stats_svc->get_port_stats();
		vector<openflow_vlan_port> sw_ports = sw_state->get_all_switch_ports();

		for (const auto& port : sw_ports) {
			string port_name = port.get_openflow_port_const().name;
			string port_type = port.is_tagged_port() ? "tagged" : "untagged";
			string port_status = port.get_openflow_port_const().is_up() ? "UP" : "DOWN";

			// generate the vlan string
			string port_vlans;
			char vlan_buf[32];
			set<uint16_t> vlan_membership = port.get_all_vlans();
			for (const auto& vlan_id : vlan_membership) {
				sprintf(vlan_buf, "%hu ", vlan_id);
				port_vlans += vlan_buf;
			}
			
			// generate stat strings
			string tx_packets, tx_bytes, rx_packets, rx_bytes;
			char stat_buf[32];
			for (const auto& stat : port_stats) {
				if (!stat.all_ports && stat.port == port.get_openflow_port_const().port_number) {
					sprintf(stat_buf, "%" PRIu64, stat.tx_packets);
					tx_packets = stat_buf;
					sprintf(stat_buf, "%" PRIu64, stat.tx_bytes);
					tx_bytes = stat_buf;
					sprintf(stat_buf, "%" PRIu64, stat.rx_packets);
					rx_packets = stat_buf;
					sprintf(stat_buf, "%" PRIu64, stat.rx_bytes);
					rx_bytes = stat_buf;
				}
			}

			if (tx_packets.empty()) {
				tx_packets = "updating";
				tx_bytes = "updating";
				rx_packets = "updating";
				rx_bytes = "updating";
			}

			// generate port features
			auto features = port.get_openflow_port_const().current_features;
			string port_features;
			if (features.bandwidth_10gb_full_duplex) {
				port_features = "10Gbps ";
			} else if (features.bandwidth_1gb_full_duplex) {
				port_features = "1Gbps ";
			} else if (features.bandwidth_1gb_half_duplex) {
				port_features = "1Gbps half duplex ";
			} else {
				port_features = "unknown ";
			}

			if (features.medium_copper) {
				port_features += "copper";
			} else if (features.medium_fiber) {
				port_features += "fiber";
			}

			// consolidate display
			char buf[1024];
			memset(buf, ' ', sizeof(buf));
			memcpy(buf+1, port_name.c_str(), port_name.size());
			memcpy(buf+9, port_type.c_str(), port_type.size());
			memcpy(buf+19, port_status.c_str(), port_status.size());
			memcpy(buf+27, port_vlans.c_str(), port_vlans.size());
			memcpy(buf+53, tx_packets.c_str(), tx_packets.size());
			memcpy(buf+63, tx_bytes.c_str(), tx_bytes.size());
			memcpy(buf+73, rx_packets.c_str(), rx_packets.size());
			memcpy(buf+83, rx_bytes.c_str(), rx_bytes.size());
			memcpy(buf+93, port_features.c_str(), port_features.size());
			buf[110] = 0;
			
			port_menu->add_option(buf);
		}
		port_menu->set_current_selection(old_selection);
	}

	port_background->set_visible(true);
	port_background->commit_attributes();
	port_menu->set_visible(true);
	port_menu->set_enable(true);
	port_menu->commit_all();
}

// displays the ARP table
void ironstack_gui::do_arp_screen() {

	// grab pointers to needed services
	shared_ptr<arp> arp_svc = static_pointer_cast<arp>(controller->get_service(service_catalog::service_type::ARP));

	if (arp_svc == nullptr) {
		arp_menu->printfc(FG_RED | BG_BLACK, vec2d(0,0), "ARP table not available until controller is online.");
	} else {

		// get all vlans
		set<uint16_t> vlans = arp_svc->get_arp_vlans();
		int old_selection = arp_menu->get_current_selection();
		arp_menu->clear_options();

		// for each vlan, get the arp table and populate table
		for (const auto& vlan_id : vlans) {
			
			vector<arp_entry> arp_entries = arp_svc->get_arp_table(vlan_id);
			for (const auto& entry : arp_entries) {
				char stat_buf[32];
				sprintf(stat_buf, "%hu", vlan_id);

				string vlan = stat_buf;
				string ip_address_string = entry.nw_address.to_string();
				string persistent = entry.persistent ? "persistent" : "-";
				string mac_address_string = entry.dl_address.to_string();
				string vendor = dl_address_db.lookup_name_for_mac(entry.dl_address);
				if (vendor.empty()) {
					vendor = "[unknown vendor]";
				}

				// generate lookup count, last lookup and last update
				string lookup_count, last_lookup, last_updated;
				sprintf(stat_buf, "%u", entry.lookup_count);
				lookup_count = stat_buf;
				sprintf(stat_buf, "%us", entry.last_lookup.get_time_elapsed_ms()/1000);
				last_lookup = stat_buf;
				sprintf(stat_buf, "%us", entry.last_updated.get_time_elapsed_ms()/1000);
				last_updated = stat_buf;

				char buf[1024];
				memset(buf, ' ', sizeof(buf));
				memcpy(buf+1, vlan.c_str(), vlan.size());
				memcpy(buf+7, ip_address_string.c_str(), ip_address_string.size());
				memcpy(buf+24, mac_address_string.c_str(), mac_address_string.size());
				memcpy(buf+43, vendor.c_str(), vendor.size());
				memcpy(buf+69, lookup_count.c_str(), lookup_count.size());
				memcpy(buf+84, last_lookup.c_str(), last_lookup.size());
				memcpy(buf+98, last_updated.c_str(), last_updated.size());
				buf[110] = 0;

				arp_menu->add_option(buf);
			}
		}

		arp_menu->set_current_selection(old_selection);
	}

	arp_background->set_visible(true);
	arp_background->commit_attributes();
	arp_menu->set_visible(true);
	arp_menu->set_enable(true);
	arp_menu->commit_all();
}

// displays the list of CAM entries on the screen
void ironstack_gui::do_cam_screen() {

	// grab pointers to needed services
	shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
	if (cam_svc == nullptr) {
		cam_menu->printfc(FG_RED | BG_BLACK, vec2d(0,0), "CAM table not available until controller is online.");
	} else {

		// get all vlans
		int old_choice = cam_menu->get_current_selection();
		cam_menu->clear_options();

		set<uint16_t> vlans = cam_svc->get_cam_vlans();
		for (const auto& vlan_id : vlans) {

			// for each vlan id, get the cam table
			auto cam_table = cam_svc->get_cam_table(vlan_id);
			for (const auto& entry : cam_table) {

				char stat_buf[32];
				sprintf(stat_buf, "%hu", vlan_id);

				string vlan = stat_buf;
				string mac_address_string = entry.second.dl_address.to_string();
				string vendor = dl_address_db.lookup_name_for_mac(entry.second.dl_address);
				if (vendor.empty()) {
					vendor = "[unknown vendor]";
				}

				string phy_port = "???";
				if (entry.second.phy_port < OFPP_MAX) {
					// TODO -- this is a hack for the dell switches. should really lookup the port name
					sprintf(stat_buf, "Te0/%hu", entry.second.phy_port-1);
					phy_port = stat_buf;
				} else if (entry.second.phy_port == OFPP_CONTROLLER) {
					phy_port = "local";
				}

				// generate lookup count, last lookup and last update
				string lookup_count, last_lookup, last_updated;
				sprintf(stat_buf, "%u", entry.second.lookup_count);
				lookup_count = stat_buf;
				sprintf(stat_buf, "%us", entry.second.last_lookup.get_time_elapsed_ms()/1000);
				last_lookup = stat_buf;
				sprintf(stat_buf, "%us", entry.second.last_updated.get_time_elapsed_ms()/1000);
				last_updated = stat_buf;

				char buf[1024];
				memset(buf, ' ', sizeof(buf));
				memcpy(buf+1, vlan.c_str(), vlan.size());
				memcpy(buf+7, mac_address_string.c_str(), mac_address_string.size());
				memcpy(buf+26, vendor.c_str(), vendor.size());
				memcpy(buf+52, phy_port.c_str(), phy_port.size());
				memcpy(buf+60, lookup_count.c_str(), lookup_count.size());
				memcpy(buf+75, last_lookup.c_str(), last_lookup.size());
				memcpy(buf+89, last_updated.c_str(), last_updated.size());
				buf[100] = 0;

				cam_menu->add_option(buf);
			}
		}

		cam_menu->set_current_selection(old_choice);
	}

	cam_background->set_visible(true);
	cam_background->commit_attributes();
	cam_menu->set_visible(true);
	cam_menu->set_enable(true);
	cam_menu->commit_all();
}

void ironstack_gui::do_switch_control_screen() {}

// displays the experimental feature screen
void ironstack_gui::do_experimental_screen() {

	static bool waiting_for_criteria = true;
	static of_match criteria;

	// show the auto broadcast rules menu
	if (experimental_auto_broadcast_rules) {
		(static_pointer_cast<input_menu>(experimental_menu))->set_description("acceleration currently ON\n --------------------------", 2);
	} else {
		(static_pointer_cast<input_menu>(experimental_menu))->set_description("acceleration currently OFF\n --------------------------", 2);
	}

	experimental_background->set_visible(true);
	experimental_background->commit_attributes();
	experimental_menu->set_visible(true);
	experimental_menu->set_enable(true);
	experimental_menu->commit_all();

	int decision = experimental_menu->wait_for_decision(0);
	if (decision == 0) {
		experimental_auto_broadcast_rules = true;
	} else if (decision == 1) {
		experimental_auto_broadcast_rules = false;
	}

	if (experimental_auto_broadcast_rules) {
		(static_pointer_cast<input_menu>(experimental_menu))->set_description("acceleration currently ON\n --------------------------", 2);
	} else {
		(static_pointer_cast<input_menu>(experimental_menu))->set_description("acceleration currently OFF\n --------------------------", 2);
	}
	experimental_menu->commit_all();

	// show the manual rule addition menu
	experimental_rule_instructions->set_visible(true);
	experimental_rule_instructions->commit_all();
	experimental_textbox->set_visible(true);
	experimental_textbox->set_enable(true);
	experimental_textbox->commit_all();

	// get a manual rule addition command if any
	experimental_rule_instructions->clear();
	if (waiting_for_criteria) {
		experimental_rule_instructions->printf("match syntax:\n"
	    "in_port   = [n | *]\n"
	    "dl_src    = [eth addr | *]  dl_dest  = [eth addr | *]\n"
	    "vlan_id   = [n | *]         vlan_pcp = [n | *]\n"
	    "ethertype = [n | *]\n"
	    "ip_tos    = [n | *]         ip_proto = [n | *]\n"
	    "nw_src    = [ip addr]       nw_dest  = [ip addr]\n"
	    "nw_src_lsb_wildcard  = [n, 0 <= n <= 32]\n"
	    "nw_dest_lsb_wildcard = [n, 0 <= n <= 32]\n"
	    "-----------------------------------------------------");
	} else {
		experimental_rule_instructions->printf("action syntax:\n"
	    "out_port     = [n | controller]\n"
	    "set_vlan     = [n]               set_vlan_pcp  = [n]\n"
	    "strip_vlan   = [1 | 0]           set_dl_src    = [eth addr]\n"
	    "set_dl_dest  = [eth_addr]        set_nw_src    = [ip addr]\n"
	    "set_nw_dest  = [ip addr]         set_ip_tos    = [n]\n"
	    "set_src_port = [n]               set_dest_port = [n]\n"
		  "enqueue_port = [n] queue_id = [x]\n"
	    "----------------------------------------------------------");
	}
	experimental_rule_instructions->commit_output();

	auto result = experimental_textbox->wait_for_input(0);
	if (result.first) {
		if (waiting_for_criteria) {
			if (criteria.from_string(result.second)) {
				waiting_for_criteria = false;
				display->set_focus(experimental_textbox);
			} else {
				experimental_rule_instructions->clear();
				experimental_rule_instructions->printfc(FG_RED | BG_BLACK, "invalid criteria specification!");
				experimental_rule_instructions->commit_output();
				sleep(2);
			}
		} else {
			openflow_action_list actions;
			if (actions.from_string(result.second)) {
				shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(controller->get_service(service_catalog::service_type::FLOWS));
				if (flow_svc != nullptr) {
					flow_svc->add_flow_auto(criteria, actions, "manual", false, 10000);
				}
			} else {
				experimental_rule_instructions->clear();
				experimental_rule_instructions->printfc(FG_RED | BG_BLACK, "invalid action specification!");
				experimental_rule_instructions->commit_output();
				sleep(2);
			}
			waiting_for_criteria = true;
		}
	}
}

// hides all components associated with the summary screen
void ironstack_gui::hide_summary_screen() {
	summary->set_visible(false);
	summary->commit_attributes();
	cpu_utilization->set_visible(false);
	cpu_utilization->commit_attributes();
	l2_utilization->set_visible(false);
	l2_utilization->commit_attributes();
	acl_utilization->set_visible(false);
	acl_utilization->commit_attributes();
}

// hides all components associated with the flow screen
void ironstack_gui::hide_flow_screen() {
	flow_background->set_visible(false);
	flow_background->commit_attributes();
	flow_menu->set_visible(false);
	flow_menu->set_enable(false);
	flow_menu->commit_attributes();
}

// hides all components associated with the port screen
void ironstack_gui::hide_port_screen() {
	port_background->set_visible(false);
	port_background->commit_attributes();
	port_menu->set_visible(false);
	port_menu->set_enable(false);
	port_menu->commit_attributes();
}

// hides all components in the ARP table
void ironstack_gui::hide_arp_screen() {
	arp_background->set_visible(false);
	arp_background->commit_attributes();
	arp_menu->set_visible(false);
	arp_menu->set_enable(false);
	arp_menu->commit_attributes();
}

// hides all components in the CAM table
void ironstack_gui::hide_cam_screen() {
	cam_background->set_visible(false);
	cam_background->commit_attributes();
	cam_menu->set_visible(false);
	cam_menu->set_enable(false);
	cam_menu->commit_attributes();
}

// hides all components in the switch control screen (currently has nothing)
void ironstack_gui::hide_switch_control_screen() {
}

// hides all components in the experimental screen
void ironstack_gui::hide_experimental_screen() {
	experimental_background->set_visible(false);
	experimental_background->commit_attributes();
	experimental_menu->set_visible(false);
	experimental_menu->set_enable(false);
	experimental_menu->commit_attributes();
	experimental_rule_instructions->set_visible(false);
	experimental_rule_instructions->commit_attributes();
	experimental_textbox->set_visible(false);
	experimental_textbox->set_enable(false);
	experimental_textbox->commit_attributes();
}

// thread entrypoint to do periodic pings
void ironstack_gui::ping_entrypoint() {
	vector<pair<string, int>> ping_times;
	vector<ip_address> addresses;

	while(1) {

		// get switch state and pull other distributed controller information from the db
		shared_ptr<switch_state> sw_state = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
		if (sw_state != nullptr) {
			vector<aux_switch_info> all_info = sw_state->get_db().get_all_info();
			ping_times.reserve(all_info.size());
			addresses.reserve(all_info.size());
			for (const auto& info : all_info) {
				ping_times.push_back(make_pair(info.switch_name, -2));
				addresses.push_back(info.data_plane_ip);
			}

			// lock and update
			lock_guard<mutex> g(ping_lock);
			pings = ping_times;
			break;

		// if switch state isn't ready, wait until it is
		} else {
			timer::sleep_for_ms(500);
		}
	}

	// this loop handles the collection of ping times
	int size = addresses.size();
	while(1) {
		for (int counter = 0; counter < size; ++counter) {

			// ping each address
			shared_ptr<ironstack::echo_daemon> echo_daemon = static_pointer_cast<ironstack::echo_daemon>(controller->get_service(service_catalog::service_type::ECHO_SERVER));
			if (echo_daemon != nullptr) {
				ping_times[counter].second = echo_daemon->ping(addresses[counter], 1455, 1200);
			}

			// update after every ping
			{
				lock_guard<mutex> g(ping_lock);
				pings = ping_times;
			}
		}

		// wait three seconds between each cycle of ping checks
		timer::sleep_for_ms(3000);
	}
}

// helper function to print a fixed width parenthesis on an existing gui component
void ironstack_gui::print_parenthesis(gui_component& component, const string& str, uint32_t spaces, color_t color) {
	component.printf("[");
	component.printfc(color, "%*.*s", spaces, spaces, str.c_str());
	component.printf("]");
}
