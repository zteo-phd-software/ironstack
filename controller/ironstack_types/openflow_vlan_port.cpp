#include "openflow_vlan_port.h"
#include "../gui/output.h"

// default constructor creates an invalid vlan port
openflow_vlan_port::openflow_vlan_port() {
	valid = false;
	tagged = false;
}

// resets everything in the port
void openflow_vlan_port::clear() {
	valid = false;
	tagged = false;
	vlan_flood_port_membership.clear();
	vlan_stp_membership.clear();
	vlan_membership.clear();
	port.clear();
}

// sets the valid bit in this entry
void openflow_vlan_port::set_valid(bool state) {
	valid = state;
}

// sets the tagging on the vlan port
void openflow_vlan_port::set_vlan_tagging(bool state) {
	tagged = state;
	if (!tagged && vlan_membership.size() > 1) {
		output::log(output::loglevel::ERROR, "openflow_vlan_port::set_vlan_tagging() untagged port "
			"cannot belong to more than one vlan!\n");
		abort();
	}
}

// adds this port to a vlan flood domain
// automatically implies vlan membership
void openflow_vlan_port::add_as_flood_port_for_vlan(uint16_t vlan_id) {
	vlan_flood_port_membership.insert(vlan_id);
	add_to_vlan_membership(vlan_id);
}

// adds this port to the stp domain
// automatically implies vlan membership
void openflow_vlan_port::add_as_stp_port_for_vlan(uint16_t vlan_id) {
	vlan_stp_membership.insert(vlan_id);
	add_to_vlan_membership(vlan_id);
}

// adds this port to a vlan
void openflow_vlan_port::add_to_vlan_membership(uint16_t vlan_id) {
	vlan_membership.insert(vlan_id);
	if (!tagged && vlan_membership.size() > 1) {
		output::log(output::loglevel::ERROR, "openflow_vlan_port::add_to_vlan_membership() untagged port"
			" cannot belong to more than one vlan!\n");
		abort();
	}
}

// removes this port from a vlan flood domain
void openflow_vlan_port::remove_as_flood_port_for_vlan(uint16_t vlan_id) {
	vlan_flood_port_membership.erase(vlan_id);
}

// removes this port from a vlan stp domain
void openflow_vlan_port::remove_as_stp_port_for_vlan(uint16_t vlan_id) {
	vlan_stp_membership.erase(vlan_id);
}

// removes this port from a vlan
// automatically implies removal from flood and stp domains
void openflow_vlan_port::remove_from_vlan_membership(uint16_t vlan_id) {
	vlan_membership.erase(vlan_id);
	remove_as_flood_port_for_vlan(vlan_id);
	remove_as_stp_port_for_vlan(vlan_id);
}

// remove from all vlans
void openflow_vlan_port::remove_from_all_vlans() {
	vlan_flood_port_membership.clear();
	vlan_stp_membership.clear();
	vlan_membership.clear();
}

// sets up the openflow port
void openflow_vlan_port::set_openflow_port(const openflow_port& port_) {
	port = port_;
}

// checks if the entry is valid
bool openflow_vlan_port::is_valid() const {
	return valid;
}

// checks if the port is a tagged port
bool openflow_vlan_port::is_tagged_port() const {
	return tagged;
}

// checks if a port is part of a vlan flood domain
bool openflow_vlan_port::is_flood_port_for_vlan(uint16_t vlan_id) const {
	return vlan_flood_port_membership.count(vlan_id) > 0;
}

// checks if a port is part of a vlan spanning tree
bool openflow_vlan_port::is_stp_port_for_vlan(uint16_t vlan_id) const {
	return vlan_stp_membership.count(vlan_id) > 0;
}

// checks if a port is part of a vlan
bool openflow_vlan_port::is_member_of_vlan(uint16_t vlan_id) const {
	return vlan_membership.count(vlan_id) > 0;
}

// gets the list of all vlans that this port is a member of
set<uint16_t> openflow_vlan_port::get_all_vlans() const {
	return vlan_membership;
}

// returns a reference to the internal openflow port 
openflow_port& openflow_vlan_port::get_openflow_port() {
	return port;
}

// returns a const reference to the internal openflow port
const openflow_port& openflow_vlan_port::get_openflow_port_const() const {
	return port;
}

// returns a readable version of the object
string openflow_vlan_port::to_string() const {
	string result;

	char buf[16];
	result += string("openflow vlan port");
	result += string("\nvalid          : ") + (valid ? string("yes") : string("no"));
	result += string("\ntagged         : ") + (tagged ? string("yes") : string("no"));
	result += string("\nvlan membership: ");
	for (const auto& v : vlan_membership) {
		sprintf(buf, "%hu ", v);
		result += buf;
	}

	result += string("\nflood ports    : ");
	for (const auto& p : vlan_flood_port_membership) {
		sprintf(buf, "%hu ", p);
		result += buf;
	}
	result += string("\nstp ports      : ");
	for (const auto& p : vlan_stp_membership) {
		sprintf(buf, "%hu ", p);
		result += buf;
	}
	result += string("\nport description is as follows:");
	result += port.to_string();

	return result;
}
