#pragma once

#include <memory>
#include "../hal/hal.h"
#include "../services/switch_state.h"
#include "../openflow_messages/of_message_factory.h"
#include "../openflow_messages/of_message_barrier_reply.h"
#include "../openflow_messages/of_message_barrier_request.h"
#include "../openflow_messages/of_message_echo_reply.h"
#include "../openflow_messages/of_message_echo_request.h"
#include "../openflow_messages/of_message_error.h"
#include "../openflow_messages/of_message_features_reply.h"
#include "../openflow_messages/of_message_features_request.h"
#include "../openflow_messages/of_message_flow_removed.h"
#include "../openflow_messages/of_message_get_config_reply.h"
#include "../openflow_messages/of_message_get_config_request.h"
#include "../openflow_messages/of_message_hello.h"
#include "../openflow_messages/of_message_modify_flow.h"
#include "../openflow_messages/of_message_packet_in.h"
#include "../openflow_messages/of_message_packet_out.h"
#include "../openflow_messages/of_message_port_modification.h"
#include "../openflow_messages/of_message_port_status.h"
#include "../openflow_messages/of_message_queue_get_config_reply.h"
#include "../openflow_messages/of_message_queue_get_config_request.h"
#include "../openflow_messages/of_message_set_config.h"
#include "../openflow_messages/of_message_stats_reply.h"
#include "../openflow_messages/of_message_vendor.h"
#include "../../common/tcp.h"

class cam;
class switch_state;

namespace ironstack {
namespace net_utils {

	// reads out an integral number of bytes corresponding to the next full openflow message
	// and then deserializes it into the appropriate message subclass.
	shared_ptr<of_message> get_next_openflow_message(tcp& connection, autobuf& sock_buf);

	// for a given ingress packet, get the vlan associated with the packet. sometimes
	// the packet may come encoded with the vlan. othertimes the vlan has to be deduced
	// by looking at which untagged port it belongs to in the switch vlan.
	// TODO: does not currently handle untagged ports on untagged vlans.
	int get_vlan_from_packet(const shared_ptr<switch_state>& switch_state_svc,
		const shared_ptr<of_message_packet_in>& packet,
		const raw_packet& raw_pkt);

	// sends a packet to all flood ports (except ingress port). respects the original vlan
	// of the packet and will appropriately tag/untag packets as necessary for the
	// associated ports.
	// TODO: does not currently handle untagged ports on untagged vlans.
	void flood_packet(hal* controller,
		const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt,
		const shared_ptr<cam>& cam_svc,
		const shared_ptr<switch_state>& switch_state_svc);

	// sends a packet to all flood ports. respects the original vlan of the packet and will
	// appropriately tag/untag packets as necessary for the associated ports.
	// TODO: does not currently handle untagged ports on untagged vlans.
	void flood_packet(hal* controller,
		const shared_ptr<switch_state>& sw_state,
		const autobuf& untagged_packet_contents,
		uint16_t vlan_id);

};};

