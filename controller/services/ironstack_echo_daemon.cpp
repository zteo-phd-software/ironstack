#include "ironstack_echo_daemon.h"
#include "arp.h"
#include "cam.h"
#include "switch_state.h"
#include "../gui/output.h"
#include "../openflow_messages/of_message_factory.h"

// destructor
ironstack::echo_daemon::~echo_daemon() {
	shutdown();
}

// initializes the daemon
bool ironstack::echo_daemon::init() {
	if (initialized) return true; 

	// register callback with packet in handler
	processor = controller->get_packet_processor();
	if (processor != nullptr) {
		processor->register_filter(shared_from_this());
	} else {
		output::log(output::loglevel::WARNING, "ironstack_echo_daemon::init() -- no packet in processor. echo daemon cannot respond to packets.\n");
		return false;
	}

	output::log(output::loglevel::INFO, "ironstack echo daemon started.\n");
	initialized = true;
	return true;
}

// called after the controller has been initialized
bool ironstack::echo_daemon::init2() {
	return true;
}

// shuts down the daemon
void ironstack::echo_daemon::shutdown() {
	if (!initialized) return;
	output::log(output::loglevel::INFO, "ironstack echo daemon shutting down.\n");
	
	if (processor != nullptr) {
		processor->unregister_filter(shared_from_this());
	}

	// fail all waiting requests
	lock_guard<mutex> g(lock);
	for (const auto& it : wait_queue) {
		unique_lock<mutex> m(it.second->lock);
		it.second->status = false;
		it.second->cond.notify_all();
	}

	// clear stuff
	wait_queue.clear();
	seq = 0;
	processor = nullptr;

	output::log(output::loglevel::INFO, "ironstack echo daemon shutdown complete.\n");
	initialized = false;
}

// handles echo requests
bool ironstack::echo_daemon::filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {

	// don't process packets if not initialized
	if (!initialized) return false;

	// ignore packets that are not icmp
	if (raw_pkt.packet_type != raw_packet::packet_t::ICMP_PACKET) {
		return false;
	}

	// ignore malformed packets
	icmp_packet pkt;
	if (!pkt.deserialize(packet->pkt_data)) {
		output::log(output::loglevel::WARNING, "ironstack echo daemon::filter_packet() cannot deserialize ICMP packet.\n");
		return false;
	}

	// get switch IP and MAC address
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));
	if (switch_state_svc == nullptr) {
		output::log(output::loglevel::WARNING, "ironstack echo daemon::filter_packet() switch state offline; cannot process ICMP packets.\n");
		return false;
	}

	ip_address switch_ip = switch_state_svc->get_switch_ip();
	mac_address switch_mac = switch_state_svc->get_switch_mac();
	if (switch_ip.is_nil() || switch_mac.is_nil()) {
		output::log(output::loglevel::WARNING, "ironstack echo daemon::filter_packet() switch state has no valid IP/MAC address. cannot process ICMP packet.\n");
		return false;
	}

	// discard packets not meant for this machine
	if (pkt.dest_ip != switch_ip || pkt.dest_mac != switch_mac) {
		return false;
	}

	// if control got here, the ICMP packet is meant for this controller.
	uint32_t seq = 0;

	// process an ICMP echo reply
	if (pkt.icmp_pkt_type == 0 && pkt.code == 0 && pkt.data.size() == sizeof(uint32_t)) {

		seq = ntohl(*((const uint32_t*)pkt.data.get_content_ptr()));
		shared_ptr<echo_daemon_request> request;

		// remove the request from the map if it still exists
		{
			lock_guard<mutex> g(lock);
			auto iterator = wait_queue.find(seq);
			if (iterator == wait_queue.end()) {
				return true;
			} else {
				request = iterator->second;
				wait_queue.erase(iterator);
			}
		}

		// notify the caller
		unique_lock<mutex> m(request->lock);
		request->status = true;
		request->cond.notify_all();
		return true;


	// process an ICMP echo request
	} else if (pkt.icmp_pkt_type == 8 && pkt.code == 0) {

		// construct reply
		icmp_packet reply;
		reply.has_vlan_tag = pkt.has_vlan_tag;
		if (pkt.has_vlan_tag) {
			reply.vlan_pcp = pkt.vlan_pcp;
			reply.dei = pkt.dei;
			reply.vlan_id = pkt.vlan_id;
		}
		reply.icmp_pkt_type = 0;
		reply.code = 0;
		reply.icmp_hdr_rest = pkt.icmp_hdr_rest;
		reply.data = pkt.data;
		reply.src_ip = pkt.dest_ip;
		reply.src_mac = pkt.dest_mac;
		reply.dest_ip = pkt.src_ip;
		reply.dest_mac = pkt.src_mac;

		// construct hal reply
		autobuf serialized_msg;
		reply.serialize(serialized_msg);
		controller->send_packet(serialized_msg, packet->in_port);
		return true;
	}

	// all other packets are ignored
	return false;
}

// sends a ping request and waits for a response
// response is given in ms. -1 indicates error or timeout.
int ironstack::echo_daemon::ping(const ip_address& dest, uint16_t vlan_id, int timeout_ms) {

	if (!initialized) {
		output::log(output::loglevel::WARNING, "ironstack echo daemon offline: cannot send ping requests.\n");
		return -1;
	}

	if (timeout_ms <= 0) {
		output::log(output::loglevel::BUG, "ironstack_echo_daemon::ping() cannot ping subsystem with zero or negative timeout value. request ignored.\n");
		return -1;
	}

	// get the ARP, CAM and switch state services
	shared_ptr<arp> arp_svc = static_pointer_cast<arp>(controller->get_service(service_catalog::service_type::ARP));
	shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

	if (arp_svc == nullptr || cam_svc == nullptr || switch_state_svc == nullptr) {
		output::log(output::loglevel::ERROR, "ironstack echo daemon::ping() cannot ping when ARP, CAM or switch state services are offline.\n");
		return -1;
	}

	// get switch ip and mac address
	ip_address switch_ip = switch_state_svc->get_switch_ip();
	mac_address switch_mac = switch_state_svc->get_switch_mac();

	// trivial: if pinging the local system, don't send packet onto wire because the switch does not allow
	// an output action back to the controller port
	if (dest == switch_ip) {
		return 0;
	}

	// lookup ARP table for mac address
	mac_address dest_mac = arp_svc->lookup_mac_for(dest, vlan_id);
	if (dest_mac.is_nil()) {
//		output::log(output::loglevel::VERBOSE, "ironstack echo daemon::ping() no ARP entry for [%s]. performing ARP query.\n",
//			dest.to_string().c_str());
		dest_mac = arp_svc->send_arp_query_blocking(dest, vlan_id, timeout_ms);
	}

	// lookup physical port number for the mac address
	set<uint16_t> ports_to_use;
	int phy_port = cam_svc->lookup_port_for(dest_mac, vlan_id);
	if (phy_port == -1) {
		ports_to_use = switch_state_svc->get_flood_ports(vlan_id);
	} else {
		ports_to_use.insert(phy_port);
	}

	// setup request
	shared_ptr<echo_daemon_request> request;
	{
		lock_guard<mutex> g(lock);
		request = shared_ptr<echo_daemon_request>(new echo_daemon_request(seq++));
		wait_queue[request->seq] = request;
	}

	// construct packet and send through hal
	uint32_t local_seq = htonl(request->seq);
	icmp_packet pkt;
	pkt.icmp_pkt_type = 8;
	pkt.code = 0;
	pkt.icmp_hdr_rest = 0x01001601;
	pkt.data.inherit_read_only(&local_seq, sizeof(local_seq));
	pkt.src_ip = switch_ip;
	pkt.dest_ip = dest;
	pkt.src_mac = switch_mac;
	pkt.dest_mac = dest_mac;

	autobuf serialized_msg;
	pkt.serialize(serialized_msg);
	controller->send_packet(serialized_msg, ports_to_use);

	// block waiting for the response
	{
		unique_lock<mutex> m(request->lock);
		request->cond.wait_for(m, chrono::milliseconds(timeout_ms));
	}

	// remove from table
	{
		lock_guard<mutex> g(lock);
		wait_queue.erase(request->seq);
	}

	if (request->status) {
		return request->request_timer.get_time_elapsed_ms();
	} else {
		return -1;
	}
}

// returns service information about the ironstack echo daemon
string ironstack::echo_daemon::get_service_info() const {
	char buf[128];
	sprintf(buf, "ironstack echo daemon version %d.%d.", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the service
string ironstack::echo_daemon::get_running_info() const {
	return string();
}
