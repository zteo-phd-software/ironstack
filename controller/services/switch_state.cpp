#include "arp.h"
#include "cam.h"
#include "switch_db.h"
#include "switch_state.h"
#include "../gui/output.h"
#include "../openflow_messages/of_message_features_request.h"
#include "../openflow_messages/of_message_get_config_request.h"
#include "../openflow_messages/of_message_port_modification.h"
#include "../openflow_messages/of_message_stats_request.h"
using namespace std;

// populates switch information by looking database file
// this sets up the switch IP, mac address, and vlan ports
bool switch_state::setup_aux_info(const ip_address& addr) {

	// load schema file
	if (!db.load("config/switch_schema.csv")) {
		output::log(output::loglevel::ERROR, "switch_state::setup_aux_info() unable to load switch schema.\n");
		return false;
	}

	// lookup info from the given management ip address
	aux_switch_info info;
	if (!db.lookup_switch_info_by_management_ip(addr, info)) {
		output::log(output::loglevel::ERROR, "switch_state::setup_aux_info() lookup error -- no switch by the management plane IP %s.\n",
			addr.to_string().c_str());
		return false;
	}

	// assign a switch IP address, mac and name (TODO much later: use DHCP)
	set_switch_ip(info.data_plane_ip);

	set_switch_mac_override(false, mac_address());
	set_name(string("ironstack_") + info.switch_name);
	set_aux_switch_info(info);
//	set_switch_ip(ip_address("10.0.0.1"));


	// this code is deprecated now that autoconfig works.
	// setup VLAN configuration from file
	/*
	string vlan_config_file(string("config/") + info.switch_name + string(".cfg"));
	if (!setup_switch_vlans(vlan_config_file)) {
		output::log(output::loglevel::ERROR, "switch state: unable to load vlan config from file [%s].\n", vlan_config_file.c_str());
		return false;
	}
	*/

	return true;
}

// initializes the service
bool switch_state::init() {

	if (initialized) {
		return true;
	}

	return true;
}

// called after the controller is active and other services are online
bool switch_state::init2() {

	// retrieve switch description
	output::log(output::loglevel::INFO, "initializing switch state.\n");
	shared_ptr<of_message_stats_request_switch_description> m_desc(new of_message_stats_request_switch_description());
	shared_ptr<blocking_hal_callback> req_callback(new blocking_hal_callback());
	controller->enqueue_transaction(make_shared<hal_transaction>(m_desc, true, req_callback));
	req_callback->wait();

	// get switch features
	shared_ptr<of_message_features_request> m_features(new of_message_features_request());
	req_callback = make_shared<blocking_hal_callback>();
	controller->enqueue_transaction(make_shared<hal_transaction>(m_features, true, req_callback));
	req_callback->wait();

	// get switch configuration
	shared_ptr<of_message_get_config_request> m_config(new of_message_get_config_request());
	req_callback = make_shared<blocking_hal_callback>();
	controller->enqueue_transaction(make_shared<hal_transaction>(m_config, true, req_callback));
	req_callback->wait();


	// sanity-check controller mac/ip before inserting into CAM/ARP tables
	ip_address sw_ip = get_switch_ip();
	mac_address sw_mac = get_switch_mac();
	if (sw_ip.is_nil() || sw_ip.is_broadcast() || sw_mac.is_nil() || sw_mac.is_broadcast()) {
		output::log(output::loglevel::WARNING, "switch_state::init2() -- no IP/MAC address for controller. controller will not be reachable from data plane.\n");
		return true;
	}

	// register controller mac/ip with all CAM/ARP tables
	shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
	shared_ptr<arp> arp_svc = static_pointer_cast<arp>(controller->get_service(service_catalog::service_type::ARP));
	if (cam_svc == nullptr || arp_svc == nullptr) {
		output::log(output::loglevel::ERROR, "switch_state::init2() failed -- CAM/ARP offline.\n");
		return false;
	} else {
		cam_svc->insert_all(sw_mac, (uint16_t) OFPP_CONTROLLER);
		arp_svc->insert_all(sw_mac, sw_ip, true);
	}

	return true;
}

// shuts down the service
void switch_state::shutdown() {
	// TODO -- unregister controller mac/ip with CAM/ARP

}

// blocks until the switch is marked as ready
void switch_state::wait_until_switch_ready() {
	unique_lock<mutex> cond_lock(switch_waiters_lock);
	switch_waiters_cond.wait(cond_lock, [this](){ return initialized; });
}

// polls (non blocking) the switch to see if it has acquired its addresses
// and other necessary various switch information
bool switch_state::is_switch_ready() const {
	unique_lock<mutex> cond_lock(switch_waiters_lock);
	return initialized;
}

// sets the switch description (called from hal)
void switch_state::set_switch_description(const of_message_stats_reply_switch_description& desc) {
	lock_guard<mutex> g(lock);
	switch_description = desc.description;
	switch_description_set = true;
	output::log(output::loglevel::INFO, "switch state: switch hardware information as follows:\n%s\n", desc.to_string().c_str());
	check_init_state_unsafe();
}

// sets the switch features (called from hal)
void switch_state::set_switch_features(const of_message_features_reply& features) {
	lock_guard<mutex> g(lock);
	switch_features = features.switch_features;

	// add the ports into the switch
	vector<openflow_port> of_ports = features.physical_ports;
	for (const auto& of_port : of_ports) {
		create_or_update_openflow_vlan_port_unsafe(of_port);
	}

	// output to screen list of ports
	output::log(output::loglevel::INFO, "switch_state::set_switch_features(): the following switch ports were added:\n");
	for (const auto& of_vlan_port : ports) {
		output::log(output::loglevel::INFO, "  physical port %hu [%s]: state %s\n", of_vlan_port.second.get_openflow_port_const().port_number,
			of_vlan_port.second.get_openflow_port_const().name.c_str(),
			of_vlan_port.second.get_openflow_port_const().is_up() ? "UP" : "DOWN");
	}

	switch_features_and_ports_set = true;
	check_init_state_unsafe();
}

// sets the switch configuration (called from hal)
void switch_state::set_switch_config(const of_message_get_config_reply& config) {
	lock_guard<mutex> g(lock);
	switch_config = config.config;
	output::log(output::loglevel::INFO, "switch_state: switch configured as follows:\n%s\n", switch_config.to_string().c_str());
	switch_config_set = true;
	check_init_state_unsafe();
}

// updates openflow port status
void switch_state::update_switch_port(const of_message_port_status& status) {
	if (!initialized) {
		output::log(output::loglevel::BUG, "switch state::update_switch_port() -- cannot update state when it is not initialized.\n");
		return;
	}

	// ignore status messages for non-physical ports.
	if (status.description.port_number >= 0xff00) {
		return;
	}

	// enter critical section to check and update port statuses
	const openflow_port& of_port = status.description;
	openflow_vlan_port vlan_port_copy;
	bool need_callbacks = true;
	{
		lock_guard<mutex> guard(lock);

		// check if the port exists in the table, add it if it doesn't
		maybe_create_openflow_vlan_port_unsafe(of_port.port_number);

		// perform action depending on whether port was added, deleted or modified
		switch (status.reason) {
			case OFPPR_ADD:
			{
				// add the description of this port into the switch state
				output::log(output::loglevel::INFO, "switch_state::update_switch_port() port %hu added.\n", of_port.port_number);
				create_or_update_openflow_vlan_port_unsafe(of_port);
				break;
			}
			case OFPPR_DELETE:
			{
				// remove the port from the switch state (by marking it as invalid)
				remove_openflow_vlan_port_unsafe(of_port.port_number);
				output::log(output::loglevel::INFO, "switch_state::update_switch_port() port %hu removed.\n", of_port.port_number);
				break;
			}
			case OFPPR_MODIFY:
			{
				openflow_vlan_port& vlan_port = ports[of_port.port_number];
				openflow_port& original_port = ports[of_port.port_number].get_openflow_port();
				bool original_port_up = (vlan_port.is_valid() ? original_port.is_up() : false);
				bool new_port_up = of_port.is_up();

				if (original_port_up != new_port_up) {
					output::log(output::loglevel::INFO, "switch_state::update_switch_port() switch port %hu: status changed from %s to %s.\n",
						of_port.port_number,
						original_port_up ? "UP" : "DOWN",
						new_port_up ? "UP" : "DOWN");

					need_callbacks = true;
				} else {
					need_callbacks = false;		// no change (don't make unnecessary port mod callbacks)
				}

				// update information
				create_or_update_openflow_vlan_port_unsafe(of_port);
				break;
			}
			default:
			{
				output::log(output::loglevel::BUG, "switch state::update_switch_port() -- unknown action returned by the switch, ignoring.\n");
				return;
			}
		}

		// make a copy for the callback
		vlan_port_copy = ports[of_port.port_number];
	}

	// perform callbacks to inform other modules about port change activity outside of port mod lock
	if (need_callbacks) {
		perform_port_mod_callbacks(vlan_port_copy);
	}
}

// sets the switch's ip address. write once only
void switch_state::set_switch_ip(const ip_address& address) {
	lock_guard<mutex> g(lock);
	if (!switch_ip_address.is_nil()) {
		output::log(output::loglevel::ERROR, "switch_state::set_switch_ip() -- switch IP address already set.\n");
		abort();
	} else if (address.is_nil()) {
		output::log(output::loglevel::ERROR, "switch_state::set_switch_ip() -- input switch address is invalid.\n");
		abort();
	}

	switch_ip_address = address;
	check_init_state_unsafe();
}

// sets the override mac address for this switch. write once only
void switch_state::set_switch_mac_override(bool enabled, const mac_address& mac) {

	lock_guard<mutex> g(lock);
	if (mac_override_set) {
		output::log(output::loglevel::ERROR, "switch_state::set_switch_mac_override() -- override switch MAC already set.\n");
		abort();
	} else {
		if (enabled) {
			if (!mac.is_nil()) {
				switch_mac = mac;
				mac_override = true;
			} else {
				output::log(output::loglevel::ERROR, "switch_state::set_switch_mac_override() -- mac address invalid.\n");
				abort();
			}
		}
	}
	mac_override_set = true;
	check_init_state_unsafe();
}

// sets the name of this switch (write-once only)
void switch_state::set_name(const string& name) {
	lock_guard<mutex> g(lock);
	if (switch_name.empty() && name.size() < 40) {
		switch_name = name;
	} else {
		output::log(output::loglevel::ERROR, "switch_state::set_name() -- name already set or name too long.\n");
		abort();
	}
	check_init_state_unsafe();
}

// sets the auxiliary switch information
void switch_state::set_aux_switch_info(const aux_switch_info& info) {
	lock_guard<mutex> g(lock);
	aux_info = info;
}

// returns switch hw desc, revision, etc
openflow_switch_description switch_state::get_switch_description() const {
	lock_guard<mutex> g(lock);
	return switch_description;
}

// returns switch features (advertised capabilities, etc)
openflow_switch_features switch_state::get_switch_features() const {
	lock_guard<mutex> g(lock);
	return switch_features;
}

// returns the switch configuration (max miss_len, etc)
openflow_switch_config switch_state::get_switch_config() const {
	lock_guard<mutex> g(lock);
	return switch_config;
}

// returns the switch ip address
ip_address switch_state::get_switch_ip() const {
	lock_guard<mutex> g(lock);
	return switch_ip_address;
}

// returns the mac address of this switch (either the override, or if not overriden, then the mac address
// from the description message
mac_address switch_state::get_switch_mac() const {
	lock_guard<mutex> g(lock);
	if (mac_override) {
		return switch_mac;
	} 
	return switch_features.switch_address;
}

// returns the name of this switch as will be seen by others
string switch_state::get_name() const {
	lock_guard<mutex> g(lock);
	return switch_name;
}

// returns the auxiliary switch information as supplied by external means
aux_switch_info switch_state::get_aux_switch_info() const {
	lock_guard<mutex> g(lock);
	return aux_info;
}

// set the ports to be marked as vlan ports
void switch_state::set_vlan_ports(const set<uint16_t>& vlan_ports, uint16_t vlan_id) {

	if (vlan_ports.empty()) return;
	lock_guard<mutex> g(lock);

	// create the ports of they don't exist
	char buf[1024] = {0};
	char buf2[16];
	for (const auto& vlan_port : vlan_ports) {
		maybe_create_openflow_vlan_port_unsafe(vlan_port);
		ports[vlan_port].add_to_vlan_membership(vlan_id);
		sprintf(buf2, "[%hu] ", vlan_port);
		strcat(buf, buf2);
	}
	output::log(output::loglevel::INFO, "switch_state::set_vlan_ports() the following ports have been added to vlan %hu:\n  %s", vlan_id, buf);
}

// set up vlan tagging for a given port
void switch_state::set_vlan_port_tagging(uint16_t dest_port, bool state) {
	lock_guard<mutex> g(lock);
	maybe_create_openflow_vlan_port_unsafe(dest_port);
	ports[dest_port].set_vlan_tagging(state);

	output::log(output::loglevel::INFO, "switch_state::set_vlan_port_tagging(): port %hu is now marked as %s.\n",
		dest_port,
		state ? "tagged" : "untagged");
}

// sets up vlan tagging for a group of ports
void switch_state::set_vlan_port_tagging(const set<uint16_t> dest_ports, bool state) {
	lock_guard<mutex> g(lock);

	char buf[1024] = {0};
	char buf2[16];
	for (const auto& vlan_port : dest_ports) {
		maybe_create_openflow_vlan_port_unsafe(vlan_port);
		ports[vlan_port].set_vlan_tagging(state);
		sprintf(buf2, "[%hu] ", vlan_port);
		strcat(buf, buf2);
	}

	output::log(output::loglevel::INFO, "switch_state::set_vlan_port_tagging(): the following ports are marked as %s:\n  %s",
		state ? "tagged" : "untagged", buf);
}

// marks a set of ports as flood ports for a certain vlan id
void switch_state::set_vlan_flood_ports(const set<uint16_t>& dest_ports, uint16_t vlan_id) {
	lock_guard<mutex> g(lock);
	for (const auto& vlan_port : dest_ports) {
		maybe_create_openflow_vlan_port_unsafe(vlan_port);
		ports[vlan_port].add_as_flood_port_for_vlan(vlan_id);
	}
}

// gets the list of flood ports for a given vlan
set<uint16_t> switch_state::get_flood_ports(uint16_t vlan_id) const {
	set<uint16_t> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		if (port.second.is_valid() && port.second.is_flood_port_for_vlan(vlan_id)) {
			result.insert(port.first);
		}
	}
	return result;
}

// gets the list of stp ports for a given vlan
set<uint16_t> switch_state::get_stp_ports(uint16_t vlan_id) const {
	set<uint16_t> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		if (port.second.is_valid() && port.second.is_stp_port_for_vlan(vlan_id)) {
			result.insert(port.first);
		}
	}
	return result;
}

// gets the set of untagged ports on a given vlan
set<uint16_t> switch_state::get_untagged_ports(uint16_t vlan_id) const {
	set<uint16_t> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		if (port.second.is_valid() && port.second.is_member_of_vlan(vlan_id) && !port.second.is_tagged_port()) {
			result.insert(port.first);
		}
	}
	return result;
}

// gets the set of tagged ports on a given vlan
set<uint16_t> switch_state::get_tagged_ports(uint16_t vlan_id) const {
	set<uint16_t> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		if (port.second.is_valid() && port.second.is_member_of_vlan(vlan_id) && port.second.is_tagged_port()) {
			result.insert(port.first);
		}
	}
	return result;
}

// gets the set of all vlans used on the switch
set<uint16_t> switch_state::get_vlan_ids() const {
	set<uint16_t> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		set<uint16_t> vlan_for_this_port = port.second.get_all_vlans();
		result.insert(vlan_for_this_port.begin(), vlan_for_this_port.end());
	}
	return result;
}

// gets a vector of ports associated with a vlan
vector<openflow_vlan_port> switch_state::get_vlan_ports(uint16_t vlan_id) const {
	vector<openflow_vlan_port> result;
	lock_guard<mutex> g(lock);
	result.reserve(ports.size());
	for (const auto& port : ports) {
		if (port.second.is_valid() && port.second.is_member_of_vlan(vlan_id)) {
			result.push_back(port.second);
		}
	}
	return result;
}

// gets a vector of all switch ports
vector<openflow_vlan_port> switch_state::get_all_switch_ports() const {
	vector<openflow_vlan_port> result;
	lock_guard<mutex> g(lock);
	for (const auto& port : ports) {
		if (port.second.is_valid()) {
			result.push_back(port.second);
		}
	}
	return result;
}

// retrieve a switch port
bool switch_state::get_switch_port(uint16_t port, openflow_vlan_port& result) {
	lock_guard<mutex> g(lock);
	maybe_create_openflow_vlan_port_unsafe(port);
	result = ports[port];
	return ports[port].is_valid();
}

// set switch port status
void switch_state::set_port_enabled(uint16_t port, bool status) {

	lock_guard<mutex> g(lock);
	auto iterator = ports.find(port);
	if (iterator == ports.end() || !iterator->second.is_valid()) {
		output::log(output::loglevel::ERROR, "switch_state::set_port_enabled() cannot toggle port status "
			"on port %hu because it doesn't exist.\n", port);
		return;
	}

	// create the message
	openflow_port& of_port = iterator->second.get_openflow_port();
	shared_ptr<of_message_port_modification> m_port_mod(new of_message_port_modification());
	m_port_mod->port_no = of_port.port_number;
	m_port_mod->port_mac_address = of_port.dl_addr;
	m_port_mod->new_config = of_port.config;
	m_port_mod->new_config.port_down = !status;
	m_port_mod->change_features = false;
	controller->enqueue_transaction(shared_ptr<hal_transaction>(new hal_transaction(m_port_mod, false)));
}

// checks if a switch port is active or not
bool switch_state::is_port_enabled(uint16_t port) const {

	lock_guard<mutex> g(lock);
	auto iterator = ports.find(port);
	if (iterator == ports.end() || !iterator->second.is_valid()) {
		return false;
	} else {
		return iterator->second.is_valid() && iterator->second.get_openflow_port_const().is_up();
	}
}

// sets up a port modification callback
void switch_state::register_port_modification_callback(const shared_ptr<switch_port_modification_callbacks>& callback, uint16_t vlan_id) {
	
	lock_guard<mutex> g(callback_lock);
	auto iterator = port_mod_callbacks.begin();
	while (iterator != port_mod_callbacks.end()) {
		shared_ptr<switch_port_modification_callbacks> cb = iterator->second.lock();
		if (cb == nullptr) {
			iterator = port_mod_callbacks.erase(iterator);
			continue;
		} else if (iterator->first == vlan_id && cb == callback) {
			return;
		} else {
			++iterator;
		}
	}

	port_mod_callbacks.push_back(make_pair(vlan_id, callback));
}

// removes a port modification callback
void switch_state::unregister_port_modification_callback(const shared_ptr<switch_port_modification_callbacks>& callback, uint16_t vlan_id) {

	lock_guard<mutex> g(callback_lock);
	auto iterator = port_mod_callbacks.begin();
	while (iterator != port_mod_callbacks.end()) {
		shared_ptr<switch_port_modification_callbacks> cb = iterator->second.lock();
		if (cb == nullptr || (iterator->first == vlan_id && cb == callback)) {
			iterator = port_mod_callbacks.erase(iterator);
		} else {
			++iterator;
		}
	}
}

// returns switch database
switch_db switch_state::get_db() const {
	return db;
}

// return information pertaining to the switch state service class
string switch_state::get_service_info() const {
	char buf[128];
	sprintf(buf, "ironstack switch state module version %d.%d.", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the switch state
string switch_state::get_running_info() const {
	return string("");
}

// create a placeholder port if one doesn't already exist
void switch_state::maybe_create_openflow_vlan_port_unsafe(uint16_t port_num) {

	// don't add openflow-only ports
	if (port_num >= 0xff00) return;

	// create a placeholder entry if it doesn't yet exist
	auto iterator = ports.find(port_num);
	if (iterator == ports.end()) {
		openflow_vlan_port new_port;
		new_port.get_openflow_port().port_number = port_num;
		ports[port_num] = move(new_port);
	}
}

// updates an openflow port or creates one if it doesn't exist
void switch_state::create_or_update_openflow_vlan_port_unsafe(const openflow_port& port) {
	
	// don't add openflow-only ports
	if (port.port_number >= 0xff00) return;

	auto iterator = ports.find(port.port_number);
	if (iterator == ports.end()) {
		openflow_vlan_port new_port;
		new_port.set_valid(true);
		new_port.set_openflow_port(port);
		ports[port.port_number] = move(new_port);
	} else {
		iterator->second.get_openflow_port() = port;
		iterator->second.set_valid(true);
	}
}

// removes an openflow port by setting its valid bit to false
// does not create the entry if it doesn't exist
void switch_state::remove_openflow_vlan_port_unsafe(const uint16_t port_num) {

	if (port_num >= 0xff00) return;

	auto iterator = ports.find(port_num);
	if (iterator != ports.end()) {
		iterator->second.set_valid(false);
	}
}

// perform port modification callbacks -- warning: holds the callback lock
// so port mods cannot be unregistered from within
void switch_state::perform_port_mod_callbacks(const openflow_vlan_port& port) {
	lock_guard<mutex> g(callback_lock);
	for (const auto& it : port_mod_callbacks) {
		if (port.is_member_of_vlan(it.first)) {
			shared_ptr<switch_port_modification_callbacks> cb = it.second.lock();
			if (cb != nullptr) {
				cb->switch_port_modification_callback(port);
			}
		}
	}
}


// check if the switch has been fully initialized so dependent code can be unblocked
void switch_state::check_init_state_unsafe() {

	// check that every component has been initialized
	if (switch_name.empty() || switch_ip_address.is_nil() || !mac_override_set || !switch_config_set || !switch_description_set || !switch_features_and_ports_set) {
		bool first = true;
		if (!switch_description_set) {
			first = false;
		}

		if (!switch_config_set) {
			if (!first) {
			}
			first = false;
		}

		if (!switch_features_and_ports_set) {
			if (!first) {
			}
			first = false;
		}

		if (!mac_override_set) {
			if (!first) {
			}
			first = false;
		}

		if (switch_ip_address.is_nil()) {
			if (!first) {
			}
			first = false;
		}

		if (switch_name.empty()) {
			if (!first) {
			}
			first = false;
		}
		return;
	}

	// unblock all waiting threads

	unique_lock<mutex> cond_lock(switch_waiters_lock);
	initialized = true;
	switch_waiters_cond.notify_all();

	output::log(output::loglevel::INFO, "switch state initialized.\n");
}

// sets up the switch vlan port settings from a configuration file
bool switch_state::setup_switch_vlans(const string& filename) {

	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == nullptr) return false;
	
	bool result = true;
	char buf[32];
	int len;
	bool vlan_is_set = false;
	bool untagged = false;
	bool tagged = false;
	uint16_t id;
	uint16_t vlan=1;
	set<uint16_t> tagged_ports, untagged_ports, all_ports;

	// file format is:
	// vlan
	// vlan id1
	// tagged
	// ports
	// untagged
	// ports
	// vlan
	// vlan id2
	// ...

	while(!feof(fp)) {
		fgets(buf, sizeof(buf), fp);
		len = strlen(buf);

		if (len > 0 && buf[len-1] == '\n') {
			buf[len-1] = '\0';
		}

		// end of previous vlan section; update as needed
		if (strcmp(buf, "vlan") == 0) {
			if (!tagged_ports.empty() || !untagged_ports.empty()) {
				output::log(output::loglevel::INFO, "finished parsing vlan %hu\n", vlan);
				set_vlan_ports(all_ports, vlan);
				set_vlan_port_tagging(tagged_ports, true);
				set_vlan_port_tagging(untagged_ports, false);
				set_vlan_flood_ports(all_ports, vlan);
			}

			vlan_is_set = true;
			tagged_ports.clear();
			untagged_ports.clear();
			all_ports.clear();

		// this subsection contains all the tagged ports
		} else if (strcmp(buf, "tagged") == 0) {
			tagged = true;
			untagged = false;

		// this subsection contains all the untagged ports
		} else if (strcmp(buf, "untagged") == 0) {
			tagged = false;
			untagged = true;

		// is this a numeric value of some kind?
		} else if (sscanf(buf, "%hu", &id) == 1) {

			// check if the numeric value was for reading the vlan id section
			if (vlan_is_set) {
				vlan = id;
				vlan_is_set = false;
				output::log(output::loglevel::VERBOSE, "seen vlan %hu\n", vlan);

			// is it for a tagged port?
			} else if (tagged) {
				tagged_ports.insert(id);
				all_ports.insert(id);

			// is it for an untagged port?
			} else if (untagged) {
				untagged_ports.insert(id);
				all_ports.insert(id);

			// unknown tagging. error parsing.
			} else {
				output::log(output::loglevel::ERROR, "switch_state::setup_switch_vlan() error.\n");
				result = false;
				goto done;
			}

		} else {
			output::log(output::loglevel::ERROR, "switch_state::setup_switch_vlan() error -- could not parse tag/value in file.\n");
			result = false;
			goto done;
		}
	}

	// all parsing complete. update remainder into state as required
	if (!tagged_ports.empty() || !untagged_ports.empty()) {
		set_vlan_ports(all_ports, vlan);
		set_vlan_port_tagging(tagged_ports, true);
		set_vlan_port_tagging(untagged_ports, false);
		set_vlan_flood_ports(all_ports, vlan);
	}

done:
	fclose(fp);
	return result;
}
