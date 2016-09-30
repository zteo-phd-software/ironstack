#pragma once
#include <string>
#include "aux_switch_info.h"
#include "../../common/csv_parser.h"
using namespace std;

// provides an interface to read switch database information

class switch_db {
public:

	switch_db() {}

	bool load(const string& filename);

	// lookup functions
	bool lookup_switch_info_by_name(const string& name, aux_switch_info& ret) const;
	bool lookup_switch_info_by_mac(const mac_address& mac, aux_switch_info& ret) const;
	bool lookup_switch_info_by_management_ip(const ip_address& ip, aux_switch_info& ret) const;
	
	// looks up all control plane IP addresses. returns instance number (-1 if failure)
	int  lookup_switch_info_by_control_plane_ip(const ip_address& ip, aux_switch_info& ret) const;

	vector<aux_switch_info> get_all_info() const;

private:

	vector<aux_switch_info> info_vec;

};

