#include "aux_switch_info.h"

// resets the object
void aux_switch_info::clear() {
	valid = false;
	room.clear();
	switch_name.clear();
	switch_type.clear();
	switch_location.clear();
	management_ip.clear();
	control_plane1_ip.clear();
	control_plane2_ip.clear();
	control_plane3_ip.clear();
	hardware_address.clear();
	datapath_id.clear();
	data_plane_ip.clear();
}

// generates a human-readable version of the object
string aux_switch_info::to_string() const {
	string result;

	result  = string("switch name   : ") + switch_name;
	result += string("\nvalid         : ") + (valid ? string("yes") : string("no"));
	result += string("\nroom          : ") + room;
	result += string("\nswitch type   : ") + switch_type;
	result += string("\nlocation      : ") + switch_location;
	result += string("\nmanagement IP : ") + management_ip.to_string();
	result += string("\ncontroller1 IP: ") + control_plane1_ip.to_string();
	result += string("\ncontroller2 IP: ") + control_plane2_ip.to_string();
	result += string("\ncontroller3 IP: ") + control_plane3_ip.to_string();
	result += string("\nhw address    : ") + hardware_address.to_string();
	result += string("\ndatapath ID   : ") + datapath_id.to_string();
	result += string("\ndata plane IP: ") + data_plane_ip.to_string();

	return result;
}
