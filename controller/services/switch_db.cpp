#include "switch_db.h"
#include "../gui/output.h"

// loads the switch schema
bool switch_db::load(const string& filename) {

	csv_parser parser;
	if (!parser.load(filename.c_str())) {
		output::log(output::loglevel::ERROR, "switch_db::load() failed -- unable to parse [%s] as csv.\n", filename.c_str());
		return false;
	}

	// check columns
	if (parser.get_cols() != 12 ||
		parser(2,0) != "Room" ||
		parser(2,1) != "Switch Name" ||
		parser(2,2) != "Switch Type" ||
		parser(2,3) != "Switch Location" ||
		parser(2,4) != "Management IP" ||
		parser(2,5) != "Control Plane OFi-1" ||
		parser(2,6) != "Control Plane OFi-2" ||
		parser(2,7) != "Control Plane OFi-3" ||
		parser(2,8) != "Control Plane OFi-4" ||
		parser(2,9) != "Hardware Address" ||
		parser(2,10) != "DP id" || 
		parser(2,11) != "Data Plane address") {

		output::log(output::loglevel::ERROR, "switch_db::load() failed -- file [%s] is not a valid switch schema.\n", filename.c_str());
		return false;
	}

	// load all rows
	int num_rows = parser.get_rows();
	if (num_rows <= 4) {
		output::log(output::loglevel::ERROR, "switch_db::load() error -- switch schema [%s] has no data!\n", filename.c_str());
		return false;
	}
	info_vec.reserve(num_rows-4);
	for (int counter = 4; counter < num_rows; ++counter) {

		// skip empty lines
		if (parser(counter, 0) == "") {
			continue;
		}

		aux_switch_info info;
		info.valid = true;
		info.room = parser(counter, 0);
		info.switch_name = parser(counter, 1);
		info.switch_type = parser(counter, 2);
		info.switch_location = parser(counter, 3);
		info.management_ip = ip_address(parser(counter, 4));
		info.control_plane1_ip = ip_address(parser(counter, 5));
		info.control_plane2_ip = ip_address(parser(counter, 6));
		info.control_plane3_ip = ip_address(parser(counter, 7));
		info.hardware_address = mac_address(parser(counter, 9));
		info.datapath_id = mac_address(parser(counter, 10).substr(6));
		info.data_plane_ip = ip_address(parser(counter, 11));

		info_vec.push_back(info);
	}

	output::printf("%d entries loaded from switch database.\n", info_vec.size());
	output::printf("%s\n", info_vec[1].to_string().c_str());

	return true;
}

// returns switch information. lookup by switch name
bool switch_db::lookup_switch_info_by_name(const string& name, aux_switch_info& ret) const {

	for (const auto& info : info_vec) {
		if (info.valid && info.switch_name == name) {
			ret = info;
			return true;
		}
	}

	ret.clear();
	return false;
}

// returns switch information. lookup by switch mac address (datapath id)
bool switch_db::lookup_switch_info_by_mac(const mac_address& mac, aux_switch_info& ret) const {

	for (const auto& info : info_vec) {
		if (info.valid && info.datapath_id == mac) {
			ret = info;
			return true;
		}
	}

	ret.clear();
	return false;
}

// returns switch information. lookup by switch management IP
// update: switch management IP is extended to mean control plane IPs for any switch
bool switch_db::lookup_switch_info_by_management_ip(const ip_address& ip, aux_switch_info& ret) const {

	for (const auto& info : info_vec) {
		if (info.valid) {
			if (info.management_ip == ip
				|| info.control_plane1_ip == ip
				|| info.control_plane2_ip == ip
				|| info.control_plane3_ip == ip) {

				ret = info;
				return true;
			}
		}
	}

	ret.clear();
	return false;
}

// returns switch information. lookup by the controller address for any instance. returns instance id,
// or -1 for failure
int switch_db::lookup_switch_info_by_control_plane_ip(const ip_address& ip, aux_switch_info& ret) const {
	int result = -1;
	for (const auto& info : info_vec) {
		if (info.valid) {
			if (info.control_plane1_ip == ip) {
				ret = info;
				result = 1;
			} else if (info.control_plane2_ip == ip) {
				ret = info;
				result = 2;
			} else if (info.control_plane3_ip == ip) {
				ret = info;
				result = 3;
			}
		}
	}

	return result;
}

// gets all switch information
vector<aux_switch_info> switch_db::get_all_info() const {
	return info_vec;
}
