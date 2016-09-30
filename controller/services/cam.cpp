#include "cam.h"
#include "switch_state.h"
#include "../openflow_messages/of_message_packet_in.h"
#include "../gui/output.h"

// initializes the cam service
// preserves existing contents
bool cam::init() {
	lock_guard<mutex> g(lock);
	if (initialized) {
		output::log(output::loglevel::WARNING, "cam::init() -- service already started.\n");
		return true;
	}

	// register for packet_in callbacks
	processor->register_filter(shared_from_this());

	// service is ready
	initialized = true;
	return true;
}

// this initialization function is called after the controller is active
bool cam::init2() {
	return true;
}

// shuts down all cam activity
void cam::shutdown() {
	lock_guard<mutex> g(lock);
	if (initialized) {
		if (processor != nullptr) {
			processor->unregister_filter(shared_from_this());
		}
	}

	// service terminated
	initialized = false;
	output::log(output::loglevel::INFO, "cam::shutdown() OK service shutdown complete.\n");
}

// creates a cam table for a vlan
void cam::create_cam_table(uint16_t vlan_id) {
	lock_guard<mutex> g(lock);
	if (cam_tables.count(vlan_id) > 0) {
		output::log(output::loglevel::WARNING, "cam::create_cam_table(): cam table for vlan %hu already exists.\n", vlan_id);
		return;
	} else {
		cam_tables[vlan_id] = move(cam_table());
		output::log(output::loglevel::INFO, "cam::create_cam_table(): created new cam table for vlan %hu.\n", vlan_id);
	}
}

// deletes a table for a vlan
void cam::remove_cam_table(uint16_t vlan_id) {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator != cam_tables.end()) {
		cam_tables.erase(iterator);
		output::log(output::loglevel::INFO,"cam::remove_cam_table(): cam table for vlan %hu dropped.\n", vlan_id);
	} else {
		output::log(output::loglevel::ERROR, "cam::remove_cam_table(): cam table for vlan %hu doesn't exist.\n", vlan_id);
	}
}

// removes all cam tables
void cam::remove_all_cam_tables() {
	lock_guard<mutex> g(lock);
	output::log(output::loglevel::INFO, "cam::remove_all_cam_tables(): %lu tables dropped.\n", cam_tables.size());
	cam_tables.clear();
}

// gets the set of all vlan IDs with a cam table
set<uint16_t> cam::get_cam_vlans() const {
	lock_guard<mutex> g(lock);
	set<uint16_t> result;
	for (auto iterator = cam_tables.begin(); iterator != cam_tables.end(); ++iterator) {
		result.insert(iterator->first);
	}
	return result;
}

// clear a given cam table without removing it
void cam::clear_cam_table(uint16_t vlan_id) {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator == cam_tables.end()) {
		output::log(output::loglevel::ERROR, "cam::clear_cam_table() -- vlan %hu doesn't exist.\n", vlan_id);
	} else {
		iterator->second.clear();
		output::log(output::loglevel::INFO, "cam::clear_cam_table(): cam table for vlan %hu cleared.\n", vlan_id);
	}
}

// clear all cam tables without removing them
void cam::clear_all_tables() {
	lock_guard<mutex> g(lock);
	output::log(output::loglevel::INFO, "cam::clear_all_tables(): clearing all cam tables.\n");
	for (auto iterator = cam_tables.begin(); iterator != cam_tables.end(); ++iterator) {
		output::log(output::loglevel::INFO, "cam: clearing cam table for vlan %hu.\n", iterator->first);
		iterator->second.clear();
	}
}

// insert an entry into the right cam table
void cam::insert(const mac_address& dl_addr, uint16_t vlan_id, uint16_t phy_port) {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator == cam_tables.end()) {
		output::log(output::loglevel::ERROR, "cam::insert() ERROR cannot insert dl_addr [%s] phy_port [%hu]: no cam table for vlan %hu.\n",
			dl_addr.to_string().c_str(), phy_port, vlan_id);
	} else {
//		output::log(output::loglevel::INFO, "cam::insert() added dl_addr [%s] phy_port [%hu] to vlan %hu\n",
//			dl_addr.to_string().c_str(), phy_port, vlan_id);
		iterator->second.insert(dl_addr, phy_port);
	}
}

// insert an entry into all cam tables
void cam::insert_all(const mac_address& dl_addr, uint16_t phy_port) {
	lock_guard<mutex> g(lock);
	for (auto iterator = cam_tables.begin(); iterator != cam_tables.end(); ++iterator) {
		iterator->second.insert(dl_addr, phy_port);
		output::log(output::loglevel::INFO, "cam::insert_all() added dl_addr [%s] phy_port [%hu] to vlan %hu\n",
			dl_addr.to_string().c_str(), phy_port, iterator->first);
	}
}

// lookup port number for a mac address
int cam::lookup_port_for(const mac_address& dl_address, uint16_t vlan_id) const {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator != cam_tables.end()) {
		return iterator->second.lookup_port_for(dl_address);
	}
	return -1;
}

// delete a cam entry
void cam::remove(const mac_address& dl_addr, uint16_t vlan_id) {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator != cam_tables.end()) {
		output::log(output::loglevel::INFO, "cam::remove() removed dl_addr [%s] from vlan %hu.\n",
			dl_addr.to_string().c_str(), vlan_id);
		iterator->second.remove(dl_addr);
	} else {
		output::log(output::loglevel::ERROR, "cam::remove() -- trying to remove dl_addr [%s] from non-existent vlan %hu.\n",
			dl_addr.to_string().c_str(), vlan_id);
	}
}

// clear all entries associated with a port
void cam::remove(uint16_t phy_port) {
	lock_guard<mutex> g(lock);
	for (auto iterator = cam_tables.begin(); iterator != cam_tables.end(); ++iterator) {
		output::log(output::loglevel::INFO, "cam::remove() removing all cam entries on vlan %hu from port %hu.\n", iterator->first, phy_port);
		iterator->second.remove(phy_port);
	}
}

// retrieve a copy of the cam table for a specific vlan
map<mac_address, cam_entry> cam::get_cam_table(uint16_t vlan_id) const {
	lock_guard<mutex> g(lock);
	auto iterator = cam_tables.find(vlan_id);
	if (iterator != cam_tables.end()) {
		return iterator->second.get_cam_table();
	}
	return map<mac_address, cam_entry>();
}

// packet handling function to sniff packets and vlans
bool cam::filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {
	if (!initialized) return false;

	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	if (switch_state_svc == nullptr) {
		output::log(output::loglevel::WARNING, "cam::filter_packet() -- switch_state service offline.\n");
		return false;
	}

	// grab the port descriptor for this openflow vlan
	openflow_vlan_port vlan_port;
	if (!switch_state_svc->get_switch_port(packet->in_port, vlan_port)) {
		output::log(output::loglevel::WARNING, "cam::filter_packet() -- could not read port listing from switch state.\n");
		return false;
	}

	// check: is this packet correctly tagged for this port?
	if (raw_pkt.has_vlan_tag) {

		// tagged packet.
		// make sure vlan id of the packet matches vlan id and tagging of the port
		if (vlan_port.is_tagged_port() && vlan_port.is_member_of_vlan(raw_pkt.vlan_id)) {
			insert(raw_pkt.src_mac, raw_pkt.vlan_id, packet->in_port);
		} else {
			if (!vlan_port.is_tagged_port()) {
				output::log(output::loglevel::WARNING, "cam::filter_packet() -- packet is tagged for vlan id %hu "
					"but the input port %hu is untagged.\n",
					raw_pkt.vlan_id,
					packet->in_port);
					return true;		// drop the packet -- don't let other services see it
			} else {
				output::log(output::loglevel::WARNING, "cam::filter_packet() -- packet is tagged for vlan id %hu "
					"but the input port %hu is not a member of that vlan.\n",
					raw_pkt.vlan_id,
					packet->in_port);
				return true;			// drop the packet -- don't let other services see it
			}
		}

	} else {

		// untagged packet
		// make sure the port itself is untagged
		if (!vlan_port.is_tagged_port()) {

			// does this port have a vlan id?
			set<uint16_t> port_vlans = vlan_port.get_all_vlans();
			if (port_vlans.empty()) {
				insert(raw_pkt.src_mac, 1, packet->in_port);		// default vlan = 1
			} else {
//				output::log(output::loglevel::INFO, "learning %s --> port %hu\n", raw_pkt.src_mac.to_string().c_str(), packet->in_port);
				insert(raw_pkt.src_mac, *port_vlans.begin(), packet->in_port);
			}

		} else {

			output::log(output::loglevel::WARNING, "cam::filter_packet() -- packet is untagged but the input "
				"port %hu is tagged.\n",
				packet->in_port);
			return true;		// drop the packet -- don't let other services see it
		}
	}

	// always return false because the packet has to continue down the handling chain
	// (cam passively snoops)
//	output::log(output::loglevel::INFO, "cam::filter_packet() packet src [%s] dest [%s] no match [%s] explicit forward [%s]\n",
//		raw_pkt.src_mac.to_string().c_str(), raw_pkt.dest_mac.to_string().c_str(),
//		packet->reason_no_match ? "yes":"no", packet->reason_action ? "yes":"no");
	return false;
}

// get service class information
string cam::get_service_info() const {
	char buf[128];
	sprintf(buf, "generic ironstack vlan cam table version %d.%d.", service_major_version, service_minor_version);
	return string(buf);
}

string cam::get_running_info() const {
	lock_guard<mutex> g(lock);
	char buf[128];
	sprintf(buf, "irontack cam service. number of tables: %lu. ", cam_tables.size());
	return string(buf);
}

