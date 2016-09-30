#include <execinfo.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include "hal/hal.h"
#include <fcntl.h>
#include "gui/output.h"
#include "ironstack_gui.h"
#include "services/arp.h"
#include "services/cam.h"
#include "services/flow_service.h"
#include "services/flow_policy_checker.h"
#include "services/ironstack_echo_daemon.h"
#include "services/operational_stats.h"
#include "services/switch_db.h"
#include "services/dell_s48xx_l2_table.h"
#include "services/dell_s48xx_acl_table.h"
#include "switch_commander/switch_commander.h"
#include "utils/ethernet_mac_db.h"
#include "../common/csv_parser.h"
#include "../common/stacktrace.h"
#include "../common/switch_telnet.h"
using namespace std;

// controller and services here
shared_ptr<hal> controller;
shared_ptr<cam> cam_service;
shared_ptr<arp> arp_service;
shared_ptr<switch_state> sw_state;
shared_ptr<flow_service> flow_svc;
shared_ptr<ironstack::echo_daemon> echo_daemon;
shared_ptr<operational_stats> op_stats;
shared_ptr<flow_policy_checker> flow_policy_svc;

// flow tables here (for gui to access)
shared_ptr<dell_s48xx_l2_table> l2_table;
shared_ptr<dell_s48xx_acl_table> acl_table;

// for debug use
string switch_name;

// experimental global vars
bool experimental_auto_broadcast_rules = true;
bool experimental_auto_broadcast_rules_installed = false;

// global mac address lookup service (read-only)
ethernet_mac_db dl_address_db;

// executive entrypoint
int main(int argc, char** argv) {

	// enable stack trace
	stacktrace::enable();

	output::printf("ironstack v5 controller.\n");

	// parse command line arguments
	ip_address sw_management_addr;
	bool preserve_flows = false;
	int instance_id = 0;
	if (argc < 3 || argc > 4) {
		output::printf("usage: ./%s [instance id] [switch management address] [--preserve-flows]\n", argv[0]);
		return 1;
	} else {
		if (sscanf(argv[1], "%d", &instance_id) != 1 || instance_id < 1 || instance_id > 4) {
			output::printf("invalid instance id.\n");
			return 1;
		} else {
			sw_management_addr = argv[2];
			if (sw_management_addr.is_nil() || sw_management_addr.is_broadcast()) {
				output::printf("invalid management ip.\n");
				return 1;
			}
			if (argc == 4 && (strcmp(argv[3], "--preserve-flows") == 0)) {
				preserve_flows = true;
			}
		}
	}

	// parse the ethernet mac address database
	if (!dl_address_db.load_file("config/eth_vendors.txt")) {
		output::printf("error: unable to load ethernet mac address database.\n");
		return 1;
	} else {
		output::printf("%d entries loaded from the ethernet mac address database.\n", dl_address_db.get_num_entries());
	}

	// construct hal for the services
	controller = shared_ptr<hal>(new hal());

	// create services and service set for the controller
	cam_service = make_shared<cam>(controller->get_service_catalog());
	arp_service = make_shared<arp>(controller->get_service_catalog());
	sw_state = make_shared<switch_state>(controller->get_service_catalog());
	flow_svc = make_shared<flow_service>(controller->get_service_catalog());
	echo_daemon = make_shared<ironstack::echo_daemon>(controller->get_service_catalog());
	op_stats = make_shared<operational_stats>(controller->get_service_catalog());
	flow_policy_svc = make_shared<flow_policy_checker>(controller->get_service_catalog());
	set<shared_ptr<service>> services = { cam_service, arp_service, sw_state, flow_svc, echo_daemon, op_stats, flow_policy_svc };

	// setup basic information in the switch state
	if (!sw_state->setup_aux_info(sw_management_addr)) {
		output::printf("error: unable to setup switch information from the given switch address.\n");
		return 1;
	}
	switch_name = sw_state->get_name();

	// use switch commander to download switch configuration information for this vlan
	output::printf("auto-config: instance %d. connecting to [%s] to retrieve configuration.\n", instance_id, sw_management_addr.to_string().c_str());
	switch_commander commander;
	unique_ptr<switch_telnet> telnet_dev(new switch_telnet());
	if (!commander.init(move(telnet_dev), ip_port(sw_management_addr, 23), "ironman", "marvelous")) {
		output::printf("failed to retrieve information. this may be a bug. please file a ticket if you believe the management IP was correct.\n");
		return 1;
	} else {
		output::printf("telnet login success on [%s], downloading configuration.\n", sw_management_addr.to_string().c_str());
	}
	vector<vlan_set> vlan_settings;
	if (!commander.get_vlan_settings(instance_id, vlan_settings)) {
		output::log(output::loglevel::BUG, "failed to download configuration. parse issue? this may be a bug, please file a ticket.\n");
		return 1;
	}

	// display vlan settings
	for (const auto& vlan : vlan_settings) {
		if (vlan.active) {

			output::printf("configuring vlan %hu [%s].\n", vlan.vlan_id, vlan.description.c_str());
			set<uint16_t> all_ports;
			all_ports.insert(vlan.tagged_ports.begin(), vlan.tagged_ports.end());
			all_ports.insert(vlan.untagged_ports.begin(), vlan.untagged_ports.end());

			if (!all_ports.empty()) {
				sw_state->set_vlan_ports(all_ports, vlan.vlan_id);
			}
			if (!vlan.tagged_ports.empty()) {
				sw_state->set_vlan_port_tagging(vlan.tagged_ports, true);
			}
			if (!vlan.untagged_ports.empty()) {
				sw_state->set_vlan_port_tagging(vlan.untagged_ports, false);
			}
			if (!all_ports.empty()) {
				sw_state->set_vlan_flood_ports(all_ports, vlan.vlan_id);
			}
		} else {
			output::printf("skipped configuring inactive vlan %hu [%s] for this controller.\n", vlan.vlan_id, vlan.description.c_str());
		}
	}

	// 2. with vlans set up, create vlan-specific cam and arp tables
	auto switch_vlans = sw_state->get_vlan_ids();
	for (const auto& switch_vlan : switch_vlans) {
		cam_service->create_cam_table(switch_vlan);
		arp_service->create_arp_table(switch_vlan);
	}

	// 3. attach flow tables
	l2_table = make_shared<dell_s48xx_l2_table>();
	acl_table = make_shared<dell_s48xx_acl_table>();
	l2_table->set_max_capacity(10000);
	acl_table->set_max_capacity(100);
	flow_svc->attach_flow_table(l2_table);
	flow_svc->attach_flow_table(acl_table);

	// 4. get the controller IP from the switch state, based on the instance id
	set<ip_address> allowed;
	aux_switch_info info = sw_state->get_aux_switch_info();
	if (instance_id == 1) {
		allowed.insert(info.control_plane1_ip);
	} else if (instance_id == 2) {
		allowed.insert(info.control_plane2_ip);
	} else if (instance_id == 3) {
		allowed.insert(info.control_plane3_ip);
	} else {
		output::printf("instance id %hu not supported.\n", instance_id);
		return 1;
	}

	// setup GUI if compiled with text GUI support
	#ifdef __TEXT_GUI
	shared_ptr<ironstack_gui> display(new ironstack_gui(controller));
	display->init();
	#endif

	// init controller
	bool status = false;
	output::log(output::loglevel::INFO, "waiting for switch %s to connect.\n", allowed.begin()->to_string().c_str());
	status = controller->init(6633, services, allowed);
	if (status) {
		output::log(output::loglevel::INFO, "handshake ok.\n");
	} else {
		output::log(output::loglevel::ERROR, "handshake failed.\n");
		return 1;
	}

	// clear flows
	if (!preserve_flows) {
		output::log(output::loglevel::INFO, "startup policy: clearing flows on switch.\n");
		sleep(2);
		flow_svc->clear_all_flows();
	} else {
		output::log(output::loglevel::INFO, "startup policy: preserving existing flows on switch.\n");
	}

	// sleep loop. also used for experimental feature test
	experimental_auto_broadcast_rules_installed = false;
	set<uint64_t> auto_broadcast_rules_cookie_ids;
	while(1) {
		timer::sleep_for_ms(1000);
		if (experimental_auto_broadcast_rules && !experimental_auto_broadcast_rules_installed) {

			// install one flood rule for each vlan
			output::log(output::loglevel::INFO, "turning on experimental auto broadcast rules.\n");

			for (const auto& vlan : vlan_settings) {
				if (vlan.active) {
					of_match criteria;
					char criteria_buf[128];
					sprintf(criteria_buf, "vlan_id=%hu dl_dest=ff:ff:ff:ff:ff:ff", vlan.vlan_id);
					criteria.from_string(criteria_buf);
					openflow_action_list actions;
					actions.from_string("out_port=65531"); //flood port
					auto_broadcast_rules_cookie_ids.insert(flow_svc->add_flow_auto(criteria, actions, "experimental bcast", true, -1));
				}
			}
			experimental_auto_broadcast_rules_installed = true;
			output::log(output::loglevel::INFO, "experimental auto broadcast rules turned on.\n");

		// remove flood rules
		} else if (!experimental_auto_broadcast_rules && experimental_auto_broadcast_rules_installed) {
			output::log(output::loglevel::INFO, "turning off experimental auto broadcast rules.\n");
			for (const auto& cookie : auto_broadcast_rules_cookie_ids) {
				flow_svc->remove_flow(cookie);
			}
			auto_broadcast_rules_cookie_ids.clear();
			experimental_auto_broadcast_rules_installed = false;
			output::log(output::loglevel::INFO, "experimental auto broadcast rules turned off.\n");
		}
	}

	controller = nullptr;
	return 0;
}
