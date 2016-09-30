#include "of_message_features_reply.h"

using namespace std;

// default constructor
of_message_features_reply::of_message_features_reply() {
	clear();
}

// clears the message
void of_message_features_reply::clear() {
	version = 1;
	msg_type = OFPT_FEATURES_REPLY;
	msg_size = sizeof(struct ofp_switch_features);
	xid = 0;

	switch_features.clear();
	physical_ports.clear();
}

// returns verbose version of this message
string of_message_features_reply::to_string() const {
	string result = switch_features.to_string();
	uint32_t n_ports = physical_ports.size();
	
	for (uint32_t counter = 0; counter < n_ports; counter++) {
		result += "-----------------------------------------------------------\n";
		result += physical_ports[counter].to_string() + "\n";
	}

	return result;
}

// serializes the features reply message (should never be called)
uint32_t of_message_features_reply::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the features reply message
bool of_message_features_reply::deserialize(const autobuf& input) {
	uint32_t n_physical_ports = 0;
	uint32_t port_defn_size = 0;
	struct ofp_switch_features header;
	struct ofp_phy_port port_desc;
	uint8_t switch_addr_raw[OFP_ETH_ALEN];
	uint32_t counter;
	openflow_port current_port;

	clear();
	bool status = of_message::deserialize(input);
	if (!status) {
		goto fail;
	}

	// sanity checks for message deserialization to features reply type
	if (msg_type != OFPT_FEATURES_REPLY) {
		goto fail;
	}

	port_defn_size = msg_size - sizeof(struct ofp_switch_features);

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (port_defn_size % sizeof(struct ofp_phy_port) != 0) {
		goto fail;
	}
	#endif

	// deserialize fields in main message
	input.memcpy_to(&header, sizeof(struct ofp_switch_features));
//	memcpy(&header, input.get_content_ptr(), sizeof(struct ofp_switch_features));
	switch_features.datapath_id = header.datapath_id;
	switch_features.n_buffers = ntohl(header.n_buffers);
	switch_features.n_tables = header.n_tables;

	memcpy(switch_addr_raw, ((char*)&switch_features.datapath_id)+2, 6);
	switch_features.switch_address.set_from_network_buffer(switch_addr_raw);

	switch_features.capabilities.set(ntohl(header.capabilities));
	switch_features.actions_supported.set(ntohl(header.actions));

	// deserialize physical port data
	n_physical_ports = (msg_size - sizeof(struct ofp_switch_features)) /
		(sizeof(struct ofp_phy_port));

	for (counter = 0; counter < n_physical_ports; counter++) {
		memcpy(&port_desc, ((const char*) input.get_content_ptr()) +
			sizeof(struct ofp_switch_features) +
			counter*sizeof(struct ofp_phy_port), sizeof(struct ofp_phy_port));
		current_port.clear();
		current_port.set(port_desc);
		physical_ports.push_back(current_port);
	}

	// features reply message type deserialization OK
	return true;

fail:
	clear();
	return false;
}
