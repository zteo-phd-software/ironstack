#include "arp.h"
#include "flow_policy_checker.h"
#include "../utils/openflow_utils.h"
#include "../gui/output.h"

// prepares the arp service for work before the controller is active
bool arp::init() {

	lock_guard<mutex> g(table_lock);
	if (initialized) {
		output::log(output::loglevel::WARNING, "arp::init() -- already initialized.\n");
		return true;
	}

	atomic_store(&requests_forwarded, (uint32_t) 0);
	atomic_store(&requests_serviced, (uint32_t) 0);
	atomic_store(&requests_made, (uint32_t) 0);

	// register packet processor. arp service must be generated by new() so enable
	// sharing from this is OK since the creator holds the original shared_ptr
	processor = controller->get_packet_processor();
	if (processor != nullptr) {
		processor->register_filter(shared_from_this());
	} else {
		output::log(output::loglevel::WARNING, "arp::init() -- no packet in processor! ARP service cannot respond to line packets.\n");
		return false;
	}

	output::log(output::loglevel::INFO, "arp::init() OK service started.\n");
	initialized = true;

	return true;
}

// initialization to be performed after controller is active
bool arp::init2() {
	return true;
}

// shuts down the arp service, but does not purge tables or pending queries
// TODO -- need to purge tables and pending queries
void arp::shutdown() {

	{
		// stop the service
		lock_guard<mutex> g(table_lock);
		initialized = false;
	}

	{
		// halt existing queries
		lock_guard<mutex> g(query_lock);
		pending_queries.clear();
	}

	// unregister packet processor
	if (processor != nullptr) {
		processor->unregister_filter(shared_from_this());
	}

	output::log(output::loglevel::INFO, "arp::shutdown() OK service shutdown complete.\n");
}

// creates an arp table for a specified vlan
void arp::create_arp_table(uint16_t vlan_id) {
	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		return;
	} else {
		arp_tables[vlan_id] = arp_table();
		output::log(output::loglevel::INFO, "arp_table: created ARP table for vlan id %hu.\n", vlan_id);
	}
}

// removes an arp table
void arp::remove_arp_table(uint16_t vlan_id) {
	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		arp_tables.erase(iterator);
	}
}

// removes all arp tables
void arp::remove_all_arp_tables() {
	lock_guard<mutex> g(table_lock);
	arp_tables.clear();
}

// gets the list of all vlans for the arp service
set<uint16_t> arp::get_arp_vlans() const {
	lock_guard<mutex> g(table_lock);
	set<uint16_t> result;
	for (const auto& it : arp_tables) {
		result.insert(it.first);
	}
	return result;
}

// clear non persistent arp entries in a vlan
void arp::clear_arp_table(uint16_t vlan_id) {
	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		iterator->second.clear();
	}
}

// clear all non persistent entries in all vlans
void arp::clear_all_tables() {
	lock_guard<mutex> g(table_lock);
	for (auto& it : arp_tables) {
		it.second.clear();
	}
}

// insert an entry into an arp table
void arp::insert(const mac_address& dl_address, const ip_address& nw_address,
	uint16_t vlan_id, bool persistent) {
	lock_guard<mutex> g(table_lock);

	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		if (persistent) {
			iterator->second.insert_persistent(dl_address, nw_address);
		} else {
			iterator->second.insert(dl_address, nw_address);
		}
	} else {
		output::log(output::loglevel::WARNING, "arp::insert() -- [%s]:[%s] not inserted (vlan %hu not found).\n",
			dl_address.to_string().c_str(),
			nw_address.to_string().c_str(),
			vlan_id);
	}
}

// insert an entry into all arp tables
void arp::insert_all(const mac_address& dl_address, const ip_address& nw_address,
	bool persistent) {

	output::log(output::loglevel::INFO, "arp::insert_all() -- added dl_addr [%s] <---> [%s].\n",
		dl_address.to_string().c_str(),
		nw_address.to_string().c_str());

	lock_guard<mutex> g(table_lock);
	for (auto& it : arp_tables) {
		if (persistent) {
			it.second.insert_persistent(dl_address, nw_address);
		} else {
			it.second.insert(dl_address, nw_address);
		}
	}
}

// lookup an arp entry
ip_address arp::lookup_ip_for(const mac_address& dl_address, uint16_t vlan_id) const {

	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator == arp_tables.end()) {
		return ip_address();
	} else {
		return iterator->second.lookup_ip_for(dl_address);
	}
}

// lookup an arp entry
mac_address arp::lookup_mac_for(const ip_address& nw_address, uint16_t vlan_id) const {

	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator == arp_tables.end()) {
		return mac_address();
	} else {
		return iterator->second.lookup_mac_for(nw_address);
	}
}

// remove an arp entry
void arp::remove(const mac_address& dl_address, uint16_t vlan_id) {

	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		iterator->second.remove(dl_address);
	}
}

// remove an arp entry
void arp::remove(const ip_address& nw_address, uint16_t vlan_id) {

	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		iterator->second.remove(nw_address);
	}
}

// remove an arp entry from all tables
void arp::remove_all(const mac_address& dl_address) {

	lock_guard<mutex> g(table_lock);
	for (auto& it : arp_tables) {
		it.second.remove(dl_address);
	}
}

// remove an arp entry from all tables
void arp::remove_all(const ip_address& nw_address) {
	lock_guard<mutex> g(table_lock);
	for (auto& it : arp_tables) {
		it.second.remove(nw_address);
	}
}

// get a copy of the arp table for a given vlan
vector<arp_entry> arp::get_arp_table(uint16_t vlan_id) const {
	lock_guard<mutex> g(table_lock);
	auto iterator = arp_tables.find(vlan_id);
	if (iterator != arp_tables.end()) {
		return iterator->second.get_arp_table();
	} else {
		return vector<arp_entry>();
	}
}

// sends an ARP query (floods all stp ports)
void arp::send_arp_query(const ip_address& dest, uint16_t vlan_id) {

	if (!initialized) {
		output::log(output::loglevel::ERROR, "arp::send_arp_query() -- arp service not initialized.\n");
		return;
	}

	// sanity check -- don't ARP for broadcast or nil addresses
	if (dest.is_broadcast()) {
//		output::log(output::loglevel::VERBOSE, "arp::send_arp_query() -- arp request for broadcast address [%s] dropped.\n", dest.to_string().c_str());
		return;
	} else if (dest.is_nil()) {
//		output::log(output::loglevel::VERBOSE, "arp::send_arp_query() -- arp request for null address [%s] dropped.\n", dest.to_string().c_str());
		return;
	}

	// grab some data needed for the arp query to work
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	if (switch_state_svc == nullptr) {
		output::log(output::loglevel::WARNING, "arp::send_arp_query() -- switch state offline; cannot get STP flood ports.\n");
		return;
	}
	mac_address switch_mac = switch_state_svc->get_switch_mac();
	ip_address switch_ip = switch_state_svc->get_switch_ip();

	// it would be silly to arp query our own machine, so we ignore requests for that
	if (dest == switch_ip) {
		return;
	}

	// synthesize ARP request
	arp_packet query;
	query.src_mac = switch_mac;
	query.sender_mac = query.src_mac;
	query.dest_mac = mac_address("ff:ff:ff:ff:ff:ff");
	query.receiver_mac = mac_address("00:00:00:00:00:00");
	query.arp_request = true;
	query.src_ip = switch_ip;
	query.dest_ip = dest;

	autobuf serialized_query;
	query.serialize(serialized_query);

	// send it out
//	output::log(output::loglevel::VERBOSE, "arp::send_arp_query() -- broadcasting arp query for [%s].\n", dest.to_string().c_str());
	ironstack::net_utils::flood_packet(controller, switch_state_svc, serialized_query, vlan_id);
	requests_made++;
}

// sends an arp query and waits for it to complete or timeout
// timeout_ms must be non-negative. if set to 0, will check the table once and return results if they are there
mac_address arp::send_arp_query_blocking(const ip_address& dest_ip, uint16_t vlan_id, int timeout_ms) {

	if (!initialized) {
		output::log(output::loglevel::ERROR, "arp::send_arp_query() -- arp service not initialized.\n");
		return mac_address();
	}

	if (timeout_ms < 0) {
		output::log(output::loglevel::ERROR, "arp::send_arp_query() -- cannot call this function with negative timeout (no infinite wait).\n");
		return mac_address();
	}

	// setup new entry/retrieve existing entry for query
	shared_ptr<pending_query_t> query;
	{
		lock_guard<mutex> g(query_lock);
		auto iterator = pending_queries.find(dest_ip);
		if (iterator == pending_queries.end()) {
			query = shared_ptr<pending_query_t>(new pending_query_t());
			query->ip_to_query = dest_ip;
			pending_queries[dest_ip] = query;
			
		} else {
			query = iterator->second;
		}
	}

	// send the query request
	send_arp_query(dest_ip, vlan_id);

	// wait until query is complete or timed out
	unique_lock<mutex> m(query->lock);
	query->cond.wait_for(m, chrono::milliseconds(timeout_ms));
	if (!query->status) {
		{
			lock_guard<mutex> g(query_lock);
			auto iterator = pending_queries.find(dest_ip);
			if (iterator != pending_queries.end()) {
				pending_queries.erase(iterator);
			}
		}

		return mac_address();
	} else {
		return query->result_mac;
	}
}

// returns the number of arp requests seen on this switch
uint32_t arp::get_requests_forwarded() const {
	return atomic_load(&requests_forwarded);
}

// returns the number of arp requests that were replied to by this service
uint32_t arp::get_requests_serviced() const {
	return atomic_load(&requests_serviced);
}

// returns the number of arp requests made through this service
uint32_t arp::get_requests_made() const {
	return atomic_load(&requests_made);
}

// packet_in handler for all ARP packets
bool arp::filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {

	// don't handle packets if service is not initialized
	if (!initialized) {
		return false;
	}

	// check if this is an ARP packet
	if (raw_pkt.packet_type != raw_packet::ARP_PACKET) {

		// if it's an ICMP/TCP/UDP packet, grab the IPv4 address!
		ip_address dest_ip;
		if (raw_pkt.packet_type == raw_packet::ICMP_PACKET) {
			icmp_packet icmp_pkt;
			if (icmp_pkt.deserialize(packet->pkt_data)) {
				dest_ip = icmp_pkt.dest_ip;
			}
		} else if (raw_pkt.packet_type == raw_packet::TCP_PACKET) {
			tcp_packet tcp_pkt;
			if (tcp_pkt.deserialize(packet->pkt_data)) {
				dest_ip = tcp_pkt.dest_ip;
			}
		} else if (raw_pkt.packet_type == raw_packet::UDP_PACKET) {
			udp_packet udp_pkt;
			if (udp_pkt.deserialize(packet->pkt_data)) {
				dest_ip = udp_pkt.dest_ip;
			}
		}

		// we know the source MAC address, now determine the destination MAC address
		if (!dest_ip.is_nil()) {
			shared_ptr<switch_state> sw_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
			if (sw_state_svc != nullptr) {
				int vlan_id = ironstack::net_utils::get_vlan_from_packet(sw_state_svc, packet, raw_pkt);
				if (vlan_id != -1 && lookup_mac_for(dest_ip, vlan_id).is_nil()) {
					send_arp_query(dest_ip, vlan_id);
				}
			}
		}
		return false;
	}

	// generate the ARP packet; return if malformed
	arp_packet arp_pkt;
	if (!arp_pkt.deserialize(packet->pkt_data)) {
		output::log(output::loglevel::ERROR, "arp::filter_packet() -- incoming ARP packet is malformed.\n");
		return false;
	}

	// get the updated switch IP address
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	ip_address switch_ip_address;
	mac_address switch_mac_address;
	if (switch_state_svc == nullptr) {
		output::log(output::loglevel::WARNING, "arp::filter_packet(): switch state offline; cannot get controller IP/mac.\n");
	} else {
		switch_ip_address = switch_state_svc->get_switch_ip();
		switch_mac_address = switch_state_svc->get_switch_mac();
	}

	// drop packets that cannot be looked up
	if (arp_pkt.dest_ip.is_nil()) {
		output::log(output::loglevel::WARNING, "arp::filter_packet() lookup failed for src [%s] dest [%s]; dest is invalid. query dropped.\n",
			arp_pkt.src_ip.to_string().c_str(),
			arp_pkt.dest_ip.to_string().c_str());
		return false;

	// drop packets that look like they looped
	} else if ((!switch_ip_address.is_nil() && arp_pkt.src_ip == switch_ip_address)
		|| (!switch_mac_address.is_nil() && arp_pkt.src_mac == switch_mac_address)) {

		output::log(output::loglevel::BUG, "arp lookup: WARNING arp packet [%s] from this controller [%s]:[%s] to dest [%s]:[%s] looped!\n",
			arp_pkt.arp_request ? "request" : "reply",
			switch_ip_address.to_string().c_str(),
			switch_mac_address.to_string().c_str(),
			arp_pkt.dest_ip.to_string().c_str(),
			arp_pkt.dest_mac.to_string().c_str());
		return true;

	// if query was for an address other than the controller, learn mappings by snooping
	// then let the default packet handler forward the packet on
	} else if (arp_pkt.dest_ip != switch_ip_address) {

		// snoopy update for the source
		int vlan_id = ironstack::net_utils::get_vlan_from_packet(switch_state_svc, packet, raw_pkt);
		if (vlan_id == -1 || vlan_id == 0) {
			output::log(output::loglevel::ERROR, "arp::filter_packet() -- cannot get vlan id from packet. packet dropped.\n");
			return false;
		}
		insert(arp_pkt.src_mac, arp_pkt.src_ip, (uint16_t) vlan_id);
		accelerate_flow(arp_pkt.src_mac, (uint16_t) vlan_id, packet->in_port);
		forward_packet(packet, raw_pkt);
		return true;

	// if control gets here, the ARP packet was meant for the controller
	} else {

		// if this is an ARP reply, unblock waiting users and update table
		if (!arp_pkt.arp_request) {

			output::log(output::loglevel::VERBOSE, "arp lookup: local lookup for [%s] completed.\n",
			arp_pkt.src_ip.to_string().c_str());

			// update ARP table
			int vlan_id = ironstack::net_utils::get_vlan_from_packet(switch_state_svc, packet, raw_pkt);
			if (vlan_id == -1) {
				output::log(output::loglevel::ERROR, "arp::filter_packet() -- cannot get vlan id from packet. packet dropped.\n");
				return false;
			}
			insert(arp_pkt.src_mac, arp_pkt.src_ip, (uint16_t) vlan_id);
			
			// notify anyone waiting for this result
			shared_ptr<pending_query_t> result;
			{
				lock_guard<mutex> g(query_lock);
				auto iterator = pending_queries.find(arp_pkt.src_ip);
				if (iterator != pending_queries.end()) {
					result = iterator->second;
					pending_queries.erase(iterator);
				}
			}

			if (result != nullptr) {
				unique_lock<mutex> m(result->lock);
				result->status = true;
				result->result_mac = arp_pkt.src_mac;
				result->cond.notify_all();
			}

			// TODO -- this placement might be inappropriate when the flow policy checker is moved
			accelerate_flow(arp_pkt.src_mac, (uint16_t) vlan_id, packet->in_port);
			return true;

		// this is an ARP request for this controller
		} else {

			// update the source address into the ARP table
			int vlan_id = ironstack::net_utils::get_vlan_from_packet(switch_state_svc, packet, raw_pkt);
			if (vlan_id == -1) {
				output::log(output::loglevel::ERROR, "arp::filter_packet() -- cannot get vlan id from packet. packet dropped.\n");
				return true;
			}
			insert(arp_pkt.src_mac, arp_pkt.src_ip, (uint16_t) vlan_id);

			output::log(output::loglevel::VERBOSE, "arp: responding to ARP probe from [%s].\n",
				arp_pkt.src_ip.to_string().c_str());

			// TODO -- this part can be optimized by keeping a premade arp response (the only thing that
			// changes is the receiver/dest mac and the output physical port number)
			arp_packet response;
			response.has_vlan_tag = true;
			response.vlan_id = vlan_id;
			response.src_mac = switch_mac_address;
			response.dest_mac = arp_pkt.src_mac;
			response.sender_mac = switch_mac_address;
			response.receiver_mac = arp_pkt.src_mac;
			response.arp_request = false;
			response.src_ip = switch_ip_address;
			response.dest_ip = arp_pkt.src_ip;
			autobuf serialized_response;
			response.serialize(serialized_response);

			controller->send_packet(serialized_response, packet->in_port);
			requests_serviced++;

			accelerate_flow(arp_pkt.src_mac, (uint16_t) vlan_id, packet->in_port);
			return true;
		}
	}
}

// returns information about the arp service
string arp::get_service_info() const {
	char buf[128];
	sprintf(buf, "generic ironstack vlan arp table version %d.%d.", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the arp service
string arp::get_running_info() const {

	return string();
}

// helper function to call flow policy checker that accelerates flows
bool arp::accelerate_flow(const mac_address& src_mac, uint16_t vlan_id, uint16_t phy_port) {

	shared_ptr<flow_policy_checker> fpc = static_pointer_cast<flow_policy_checker>(controller->get_service(service_catalog::service_type::FLOW_POLICY_CHECKER));

	if (fpc == nullptr) {
		output::log(output::loglevel::ERROR, "arp::accelerate_flows() -- cannot accelerate flow. flow policy checker service offline.\n");
		return false;
	}

	return fpc->accelerate_flow(src_mac, vlan_id, phy_port);
}

// helper function to call flow policy checker for forwarding packets
bool arp::forward_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {
	
	shared_ptr<flow_policy_checker> fpc = static_pointer_cast<flow_policy_checker>(controller->get_service(service_catalog::service_type::FLOW_POLICY_CHECKER));

	if (fpc == nullptr) {
		output::log(output::loglevel::ERROR, "arp::forward_packet() -- cannot forward packet. flow policy checker service offline.\n");
		return false;
	}

	return fpc->forward_packet(packet, raw_pkt);
}
