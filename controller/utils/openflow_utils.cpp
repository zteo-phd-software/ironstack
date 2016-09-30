#include "openflow_utils.h"
#include "../gui/output.h"

// gets the next openflow message from the socket and deserializes it
shared_ptr<of_message> ironstack::net_utils::get_next_openflow_message(tcp& connection, autobuf& buf) {

	// read in the common openflow header
	if (!connection.recv_fixed_bytes(buf, sizeof(struct ofp_header))) {
		output::log(output::loglevel::ERROR, "net_utils::get_next_openflow_message() unable to read msghdr from connection.\n");
		return nullptr;
	}

	// read in the balance of the openflow message
	uint32_t bytes_to_read = ntohs(((const struct ofp_header*)(buf.get_content_ptr()))->length) - sizeof(struct ofp_header);
	if (bytes_to_read == 0) {

		// deserialize buffer
		return of_message_factory::deserialize_message(buf);

	} else {
		
		// read remaining bytes
		buf.create_empty_buffer(bytes_to_read + sizeof(struct ofp_header), false);
		if (!connection.recv_fixed_bytes(buf.ptr_offset_mutable(sizeof(struct ofp_header)), bytes_to_read)) {
			output::log(output::loglevel::ERROR, "net_utils::get_next_openflow_message() unable to read msgbody from connection.\n");
			return nullptr;
		}

		// deserialize buffer
		return of_message_factory::deserialize_message(buf);
	}
}

// gets the vlan tag from a given ingress packet. consults switch state for information if
// the packet is untagged. checks for errors.
int ironstack::net_utils::get_vlan_from_packet(const shared_ptr<switch_state>& switch_state_svc,
	const shared_ptr<of_message_packet_in>& packet,
	const raw_packet& raw_pkt) {

  // if the packet is tagged, automatically return the vlan tag
  if (raw_pkt.has_vlan_tag) {
    return raw_pkt.get_vlan_id();
  }

  // if the packet is untagged, check the port tag settings
  openflow_vlan_port vlan_port;
  if (!switch_state_svc->get_switch_port(packet->in_port, vlan_port)) {
    output::log(output::loglevel::ERROR, "ironstack::net_utils::get_vlan_from_packet() could not locate port %hu in the switch state.\n", packet->in_port);
    return -1;
  }

  // an untagged packet on a tagged port is not allowed!
  if (vlan_port.is_tagged_port()) {
    output::log(output::loglevel::ERROR, "ironstack::net_utils::get_vlan_from_packet() an untagged packet has surfaced on tagged port %hu!\n", packet->in_port);
    return -1;

  } else {

    // make sure the vlan port has only one tag or we don't know which vlan to send it out on
    set<uint16_t> member_ports = vlan_port.get_all_vlans();
    if (member_ports.size() == 1) {
      return *member_ports.begin();

    } else if (member_ports.empty()) {
      output::log(output::loglevel::BUG, "ironstack::net_utils::get_vlan_from_packet() error -- untagged packet on non vlan port is not implemented. please contact the ironstack dev team.\n");
      return -1;

    } else {
      output::log(output::loglevel::ERROR, "ironstack::net_utils::get_vlan_from_packet() error -- multiple vlan ports available for an untagged packet.\n");
      return -1;
    }
  }
}

// sends a packet to all flood ports. respects original vlan of the packet.
// tags and untags packets as necessary for the respective output ports.
void ironstack::net_utils::flood_packet(hal* controller,
	const shared_ptr<of_message_packet_in>& packet,
	const raw_packet& raw_pkt,
	const shared_ptr<cam>& cam_svc,
	const shared_ptr<switch_state>& switch_state_svc) {

  // get the vlan ID of the packet, if it has any
  uint16_t pkt_vlan_id = raw_pkt.get_vlan_id();

  // get the vlan set of the ingress packet and make sure the packet is 'as expected'
  // ie. tagged with a member set vlan from a tagged port
  //     or untagged from an untagged port
  openflow_vlan_port vlan_port_info;
  if (!switch_state_svc->get_switch_port(packet->in_port, vlan_port_info)) {
    output::log(output::loglevel::ERROR, "ironstack::net_utils::flood_packet() could not locate port information from switch state. packet dropped.\n");
    return;
  }
  set<uint16_t> port_vlans = vlan_port_info.get_all_vlans();

  // verify sanity here
  if ((raw_pkt.has_vlan_tag && port_vlans.count(pkt_vlan_id) == 0)) {

    // packet has vlan tag but the port doesn't have this vlan tag
    output::log(output::loglevel::ERROR, "ironstack::net_utils::flood_packet() on ingress port %hu, packet_in has vlan tag %hu but port vlan does not include this vlan.\n", packet->in_port, pkt_vlan_id);
    output::log(output::loglevel::ERROR, "the valid vlan tags for port %hu are:\n", packet->in_port);
    for (const auto& p : port_vlans) {
      output::log(output::loglevel::ERROR, "[%hu] ", p);
    }
    output::log(output::loglevel::ERROR, "\nthe packet has been dropped from propagation.\n");
    return;

  } else if (!raw_pkt.has_vlan_tag && vlan_port_info.is_tagged_port()) {

    // packet has no vlan tag but the port is tagged
    output::log(output::loglevel::ERROR, "ironstack::net_utils::flood_packet() on ingress port %hu, packet_in is untagged but the port requires a vlan tag.\n", packet->in_port);
    output::log(output::loglevel::ERROR, "the packet has been dropped from propagation.\n");
    return;
  }

  // TODO: right now, if a port has no vlan tags and the packet is untagged, we don't handle them (they should be forwarded to
  // other non-vlan ports).
  if (!raw_pkt.has_vlan_tag && port_vlans.empty()) {
    output::log(output::loglevel::BUG, "ironstack::net_utils::flood_packet() unimplemented functionality. please contact the ironstack dev team.\n");
    return;
  }

  uint16_t actual_vlan = (raw_pkt.has_vlan_tag ? pkt_vlan_id : *port_vlans.begin());

  // TODO -- may want to consider using openflow actions to add/remove tags using hardware
  // handle untagged packet
  if (!raw_pkt.has_vlan_tag) {

		// send untagged packet as-is to untagged ports
    set<uint16_t> untagged_ports = switch_state_svc->get_untagged_ports(actual_vlan);
    untagged_ports.erase(packet->in_port);
    if (!untagged_ports.empty()) {
      controller->send_packet(packet->pkt_data, untagged_ports);
    }

		// tag packet and send tagged packet to tagged ports
    set<uint16_t> tagged_ports = switch_state_svc->get_tagged_ports(actual_vlan);
    tagged_ports.erase(packet->in_port);  // technically not necessary (since packet entered on an untagged port)
    if (!tagged_ports.empty()) {
      autobuf tagged_packet;
      std_packet_utils::set_vlan_tag(packet->pkt_data, tagged_packet, actual_vlan);

      controller->send_packet(tagged_packet, tagged_ports);
    }

  // handle tagged packet
  } else {

		// untag packet and send untagged packet to untagged ports
    set<uint16_t> untagged_ports = switch_state_svc->get_untagged_ports(actual_vlan);
    untagged_ports.erase(packet->in_port); // technically not necessary (since packet entered on tagged port)
    if (!untagged_ports.empty()) {
      autobuf untagged_packet;
      std_packet_utils::strip_vlan_tag(packet->pkt_data, untagged_packet);
      controller->send_packet(untagged_packet, untagged_ports);
    }

		// send tagged packet as-is to tagged ports
    set<uint16_t> tagged_ports = switch_state_svc->get_tagged_ports(actual_vlan);
    tagged_ports.erase(packet->in_port);
    if (!tagged_ports.empty()) {
      controller->send_packet(packet->pkt_data, tagged_ports);
    }
  }
}

// sends a packet out all flood ports for a given vlan. respects the original
// vlan of the packet and will appropriately tag/untag packets before
// sending them out the ports.
void ironstack::net_utils::flood_packet(hal* controller, const shared_ptr<switch_state>& sw_state, const autobuf& contents, uint16_t vlan_id) {
	
	set<uint16_t> tagged_ports = sw_state->get_tagged_ports(vlan_id);
	set<uint16_t> untagged_ports = sw_state->get_untagged_ports(vlan_id);

	if (!tagged_ports.empty()) {
		autobuf tagged_packet;
		std_packet_utils::set_vlan_tag(contents, tagged_packet, vlan_id);
		controller->send_packet(tagged_packet, tagged_ports);
	}
	if (!untagged_ports.empty()) {
		controller->send_packet(contents, untagged_ports);
	}

}
