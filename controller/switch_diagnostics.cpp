#define __USE_GNU
#include <execinfo.h>
#include <ucontext.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include "hal/hal.h"
#include <fcntl.h>
#include "services/switch_db.h"
#include "services/flow_service.h"
#include "../common/csv_parser.h"
#include "../common/stacktrace.h"
//#include "../common/dbg.h"
using namespace std;

// diagnostic program

// function prototypes
ip_address parse_args(const switch_db& db, aux_switch_info& info, int argc, char** argv);
void set_switch_vlan(const string& switch_name);

// some global variables
char executable_name[1024];

// controller
shared_ptr<hal> controller;

// executive entrypoint
int main(int argc, char** argv) {

	// enable stack traces
	stacktrace::enable();

	printf("ironstack switch diagnostic utility.\n");

	// load switch schema
	switch_db db;
	aux_switch_info db_switch_info;
	if (!db.load("config/switch_schema.csv")) {
		printf("error: unable to load switch schema.\n");
		return 1;
	}

	// get the address to listen on; abort if not found
	ip_address control_ip = parse_args(db, db_switch_info, argc, argv);
	if (control_ip.is_nil()) {
		return 1;
	}

	// construct hal; do handshake
	bool status = false;
	controller = shared_ptr<hal>(new hal());
	printf("waiting for switch connections.\n");
	set<ip_address> allowed;
	allowed.insert(control_ip);
	status = controller->init(6633, allowed);
	if (status) {
		printf("switch connected.\n");
	} else {
		printf("switch handshake error.\n");
		return 1;
	}

	// prepare switch state to be setup with params
	shared_ptr<switch_state> switch_state_service = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE).lock());
	if (switch_state_service == nullptr) {
		printf("could not find switch state service.\n");
		exit(1);
	}

	// setup params for the switch
	switch_state_service->set_switch_ip(ip_address("10.0.0.1"));
	switch_state_service->set_switch_mac_override(false, mac_address());
	switch_state_service->set_name("ironstack controller");
	printf("waiting for switch to be ready.\n");
	switch_state_service->wait_until_switch_ready();
	printf("switch ready.\n");
	switch_state_service->set_aux_switch_info(db_switch_info);

	// setup vlan configuration
	string vlan_config_filename(string("config/") + db_switch_info.switch_name + string(".cfg"));
	set_switch_vlan(vlan_config_filename);

	// get port information
	printf("switch port information as follows:\n");
	auto of_ports = switch_state_service->get_all_switch_ports();
	for (const auto& p : of_ports) {
		printf("%s\n\n", p.to_string().c_str());
	}

	// get flow service ready
	shared_ptr<flow_service> flow_svc;
	while(1) {
		timer::sleep_for_ms(500);
		flow_svc = static_pointer_cast<flow_service>(controller->get_service(service_catalog::service_type::FLOWS).lock());
		if (flow_svc != nullptr) {
			break;
			flow_svc->clear_all_flows();
			break;
		}
	}

	// download flows
	printf("flow information is as follows:\n");
	flow_svc->refresh_flows(3000);
	vector<openflow_flow_entry> flows = flow_svc->get_flows();
	for (const auto& x : flows) {
		printf("\n%s %s\n%s\n\n", x.is_pending_installation ? "pending" : "active", x.is_pending_removal ? " | being removed" : "", x.description.to_string().c_str());
	}
	printf("-----\n");


	controller = nullptr;
	printf("done.\n");

	return 0;
}

// parse the command line arguments and select a controller IP address
ip_address parse_args(const switch_db& db, aux_switch_info& info, int argc, char** argv) {

	ip_address result;

	// lookup by switch name
	if (argc == 3 && strcmp(argv[1], "-n") == 0) {
		if (!db.lookup_switch_info_by_name(argv[2], info)) {
			printf("error: no switch by the name of [%s]\n", argv[2]);
			return ip_address();
		} else {
			return info.control_plane3_ip;
		}
	
	// lookup by IP
	} else if (argc == 2) {

		result = argv[1];
		if (result.is_nil() || !db.lookup_switch_info_by_control_plane3_ip(result, info)) {
			printf("error: no switch by the controller OFi-3 IP address of [%s]\n", argv[1]);
			return ip_address();
		}
		return result;

	} else {
		printf("error: need to supply a switch name or IP address.\n");
		return ip_address();
	}
}


void set_switch_vlan(const string& switch_name) {

	FILE* fp = fopen(switch_name.c_str(), "rb");
	if (fp == nullptr) {
		printf("set_switch_vlan() ERROR -- could not find switch config file %s.\n", switch_name.c_str());
		abort();
	}

	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE).lock());

	char buf[32];
	int len;
	bool vlan_is_set = false;
	bool untagged = false;
	bool tagged = false;
	uint16_t id;
	uint16_t vlan=1;
	set<uint16_t> tagged_ports;
	set<uint16_t> untagged_ports;
	set<uint16_t> all_ports;

	while (!feof(fp)) {
		fgets(buf, sizeof(buf), fp);
		len = strlen(buf);

		if (len > 0 && buf[len-1] == '\n') {
			buf[len-1] = '\0';
		}

		if (strcmp(buf, "vlan") == 0) {
			vlan_is_set = true;
			tagged_ports.clear();
			untagged_ports.clear();
			all_ports.clear();

			// insert into sw state if needed
			if (!tagged_ports.empty() && !untagged_ports.empty()) {
				switch_state_svc->set_vlan_ports(all_ports, vlan);
				switch_state_svc->set_vlan_port_tagging(tagged_ports, true);
				switch_state_svc->set_vlan_port_tagging(untagged_ports, false);
				switch_state_svc->set_vlan_flood_ports(all_ports, vlan);

/*
				printf("associated vlan %hu with these ports: \n", vlan);
				for (const auto& p : all_ports) {
					printf("%hu ", p);
				}
				printf("\n");
				printf("tagged ports: \n");
				for (const auto& p : tagged_ports) {
					printf("%hu ", p);
				}
				printf("\n");
				printf("untagged ports: \n");
				for (const auto& p : untagged_ports) {
					printf("%hu ", p);
				}
				printf("\n");
*/
			}
			continue;
		} else if (strcmp(buf, "tagged") == 0) {
			tagged = true;
			untagged = false;
			continue;
		} else if (strcmp(buf, "untagged") == 0) {
			tagged = false;
			untagged = true;
			continue;
		}

		// it's a number
		if (sscanf(buf, "%hu", &id) != 1) {
			printf("error reading value.\n");
			abort();
		}

		if (vlan_is_set) {
			vlan = id;
			vlan_is_set = false;
		} else if (tagged) {
			tagged_ports.insert(id);
			all_ports.insert(id);
		} else if (untagged) {
			untagged_ports.insert(id);
			all_ports.insert(id);
		}
	}

	// insert into sw state if needed
	if (!tagged_ports.empty() && !untagged_ports.empty()) {
		switch_state_svc->set_vlan_ports(all_ports, vlan);
		switch_state_svc->set_vlan_port_tagging(tagged_ports, true);
		switch_state_svc->set_vlan_port_tagging(untagged_ports, false);
		switch_state_svc->set_vlan_flood_ports(all_ports, vlan);

/*
		printf("associated vlan %hu with these ports: \n", vlan);
		for (const auto& p : all_ports) {
			printf("%hu ", p);
		}
		printf("\n");
		printf("tagged ports: \n");
		for (const auto& p : tagged_ports) {
			printf("%hu ", p);
		}
		printf("\n");
		printf("untagged ports: \n");
		for (const auto& p : untagged_ports) {
			printf("%hu ", p);
		}
		printf("\n");
*/
	}

	fclose(fp);
}
