#include "services/switch_db.h"

int main(int argc, char** argv) {

	switch_db db;
	aux_switch_info info;
	if (!db.load("config/switch_schema.csv")) {
		printf("error: unable to read switch schema.\n");
		return 1;
	}

	// list all information
	if (argc == 2 && strcmp(argv[1], "-l") == 0) {

		auto ret = db.get_all_info();
		for (const auto& it : ret) {
			printf("%s\n\n", it.to_string().c_str());
		}

	// get info by switch name
	} else if (argc == 3 && strcmp(argv[1], "-n") == 0) {

		if (!db.lookup_switch_info_by_name(argv[2], info)) {
			printf("error: no switch by the name of [%s].\n", argv[2]);
		} else {
			printf("information is as follows:\n%s\n\n", info.to_string().c_str());
		}

	// get info by switch datapath mac address
	} else if (argc == 3 && strcmp(argv[1], "-m") == 0) {

		mac_address addr(argv[2]);
		if (!addr.is_nil() && db.lookup_switch_info_by_mac(addr, info)) {
			printf("information is as follows:\n%s\n\n", info.to_string().c_str());
		} else {
			printf("error: no switch by the datapath mac address of [%s].\n", argv[2]);
		}

	// get info by management IP
	} else if (argc == 3 && strcmp(argv[1], "-i") == 0) {
		
		ip_address addr(argv[2]);
		if (!addr.is_nil() && db.lookup_switch_info_by_management_ip(addr, info)) {
			printf("information is as follows:\n%s\n\n", info.to_string().c_str());
		} else {
			printf("error: no switch by the management IP address of [%s].\n", argv[2]);
		}

	// get info by control plane IP
	} else if (argc == 3 && strcmp(argv[1], "-c") == 0) {

		ip_address addr(argv[2]);
		int instance_id = -1;
		if (!addr.is_nil()) {
			instance_id = db.lookup_switch_info_by_control_plane_ip(addr, info);
		}

		if (instance_id != -1) {
			printf("information is as follows:\nThe specified control plane IP belongs "
				" to instance %d.\n%s\n\n", instance_id, info.to_string().c_str());
		} else {
			printf("error: no switch by the control plane IP address of [%s].\n", argv[2]);
		}

	} else {

		printf("lookup utility. options:\n");
		printf("  -l          -- lists all switches and information.\n");
		printf("  -n [name]   -- looks up switch info by name.\n");
		printf("  -m [mac]    -- looks up switch info by datapath MAC address.\n");
		printf("  -i [ip]     -- looks up switch info by switch management IP address.\n");
		printf("  -c [ip]     -- looks up switch info by control plane IP address.\n");
	}

	return 0;
}
