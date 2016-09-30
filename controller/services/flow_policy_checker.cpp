#include "flow_policy_checker.h"
#include "flow_service.h"
#include "../hal/hal.h"
#include "../utils/openflow_utils.h"
#include "../gui/output.h"

// initializes the flow policy checker service
bool flow_policy_checker::init() {
	lock_guard<mutex> g(lock);
	if (initialized) {
		output::log(output::loglevel::WARNING, "flow_policy_checker::init() already initialized.\n");
		return true;
	}

	processor->register_filter(shared_from_this());

	initialized = true;
	return true;
}

// called after the controller is initialized
bool flow_policy_checker::init2() {
	return true;
}

// shuts down the flow policy checker service
void flow_policy_checker::shutdown() {
	lock_guard<mutex> g(lock);
	initialized = false;
}

// DEVICE SPECIFIC: this is only useful for Dell S48xx switches!
// installs an accelerating rule for the src mac --> phy port pair
bool flow_policy_checker::accelerate_flow(const mac_address& src_mac, uint16_t vlan_id, uint16_t phy_port) {

	//char buf[128];
	of_match criteria;
	openflow_action_list actions;

	// set dl_dest and vlan
	criteria.wildcard_all();
	criteria.wildcard_ethernet_dest = false;
	criteria.ethernet_dest = src_mac;
	criteria.wildcard_vlan_id = false;
	criteria.vlan_id = vlan_id;

	// output to single port
	of_action_output_to_port action;
	action.send_to_controller = false;
	action.port = phy_port;
	actions.add_action(action);

	/* old code disabled
	// make this faster by instantiating the right subclasses
	sprintf(buf, "dl_dest=%s vlan_id=%hu", src_mac.to_string().c_str(), vlan_id);
	criteria.from_string(buf);
	sprintf(buf, "out_port=%hu", phy_port);
	actions.from_string(buf);
	*/

	/* debugging only
	output::log(output::loglevel::INFO, "flow_policy: installing acceleration rule for [%s] --> phy_port [%hu] on vlan [%hu]\n",
		src_mac.to_string().c_str(),
		phy_port,
		vlan_id);
	*/

	shared_ptr<flow_service> flow_svc = static_pointer_cast<flow_service>(controller->get_service(service_catalog::service_type::FLOWS));
	if (flow_svc == nullptr) {
		output::log(output::loglevel::WARNING, "flow_policy_checker::accelerate_flow() failed -- flow service offline.\n");
		return false;
	}

	return (flow_svc->add_flow_auto(criteria, actions, "l2 accelerator", false, 0)) != ((uint64_t) -1);
}

// fowards a packet after doing policy checks
bool flow_policy_checker::forward_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {

	// get pointer to cam and switch state
	shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

	if (raw_pkt.dest_mac.is_nil()) {
		//output::log(output::loglevel::WARNING, "flow_policy_checker::forward_packet() -- invalid packet type, possibly ironstack-to-ironstack. packet ignored.\n");
		return false;

	// no mac destination indicated. maybe ironstack-to-ironstack?
	// broadcast packet. forward out per broadcast rules.
	} else if (raw_pkt.dest_mac.is_broadcast()) {
	
//		output::log(output::loglevel::VERBOSE, "flow_policy_checker::forward_packet() broadcast packet received.\n");

		// broadcast packet. send on all flood ports
		// note: it isn't possible to put a generic flood or broadcast rule for this
		// without potentially violating vlan isolation. thus broadcasts need to be done
		// in software.
		ironstack::net_utils::flood_packet(controller, packet, raw_pkt, cam_svc, switch_state_svc);
		return true;

	// unicast packet. if control got here, then the switch doesn't have a flow for this
	// destination mac address. in that case, the packet has to be flooded.
	} else {
		ironstack::net_utils::flood_packet(controller, packet, raw_pkt, cam_svc, switch_state_svc);
		return true;
	}
}

// the last packet handler in the packet_in callback chain
// this checks policies and decides if an L2 rule should be installed or if the flow
// should be ignored
bool flow_policy_checker::filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {

	// ignore packet processing if service is offline
	if (!initialized) {
		return false;
	}

	// get pointer to various services
	shared_ptr<cam> cam_svc = static_pointer_cast<cam>(controller->get_service(service_catalog::service_type::CAM));
	shared_ptr<switch_state> switch_state_svc = static_pointer_cast<switch_state>(controller->get_service(service_catalog::service_type::SWITCH_STATE));

	// can't process packets if any of these services are offline
	if (cam_svc == nullptr || switch_state_svc == nullptr) {
		output::log(output::loglevel::WARNING, "flow_policy_checker::filter_packet() failed -- CAM/switch state service offline. packet ignored.\n");
		return false;
	}

	// install a flow rule for each unique source that has been seen
	if (!raw_pkt.src_mac.is_nil() && !raw_pkt.src_mac.is_broadcast()) {
		
		// construct L2 flow rule for the given source
		int vlan_id = ironstack::net_utils::get_vlan_from_packet(switch_state_svc, packet, raw_pkt);
		if (!accelerate_flow(raw_pkt.src_mac, vlan_id, packet->in_port)) {
			output::log(output::loglevel::WARNING, "flow_poicy_checker::filter_packet() -- cannot accelerate the L2 flow.\n");
		}
/*
		char buf[128];
		of_match criteria;
		openflow_action_list actions;
		sprintf(buf, "dl_dest=%s vlan_id=%hu", raw_pkt.src_mac.to_string().c_str(), vlan_id);
		criteria.from_string(buf);
		sprintf(buf, "out_port=%hu", (uint16_t) packet->in_port);
		actions.from_string(buf);

		output::log(output::loglevel::INFO, "flow_policy: installing rule for [%s] vlan [%hu] to out_port [%hu]\n",
			raw_pkt.src_mac.to_string().c_str(),
			vlan_id,
			packet->in_port);

		flow_svc->add_flow_auto(criteria, actions, false, 0);
*/
	}

	// now handle the packet by forwarding it on the the correct destination
	return forward_packet(packet, raw_pkt);
}

// returns information about the flow policy checker
string flow_policy_checker::get_service_info() const {
	char buf[128];
	sprintf(buf, "flow policy checker version %d.%d", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the flow policy checker
string flow_policy_checker::get_running_info() const {
	return string();
}
