#include "of_message.h"

using namespace std;

// default constructor
of_message::of_message(){
	clear();
}

// clears the message
void of_message::clear() {
	version = 1;
	msg_type = OFPT_HELLO;
	msg_size = sizeof(struct ofp_header);
	xid = 0;
}

// returns the type of the message, in string form
string of_message::get_message_type_string() const {
	switch (msg_type) {

		case OFPT_HELLO:
			return string("OFPT_HELLO: hello handshake (symmetric)");
			break;

		case OFPT_ERROR:
			return string("OFPT_ERROR: error diagnostic output (symmetric)");
			break;

		case OFPT_ECHO_REQUEST:
			return string("OFPT_ECHO_REQUEST: echo request (symmetric)");
			break;

		case OFPT_ECHO_REPLY:
			return string("OFPT_ECHO_REPLY: echo reply (symmetric)");
			break;

		case OFPT_VENDOR:
			return string("OFPT_VENDOR: vendor reply (symmetric)");
			break;

		case OFPT_FEATURES_REQUEST:
			return string("OFPT_FEATURES_REQUEST: features request (controller -> switch)");
			break;

		case OFPT_FEATURES_REPLY:
			return string("OFPT_FEATURES_REPLY: features reply (switch -> controller)");
			break;

		case OFPT_GET_CONFIG_REQUEST:
			return string("OFPT_GET_CONFIG_REQUEST: switch configuration request (controller -> switch)");
			break;

		case OFPT_GET_CONFIG_REPLY:
			return string("OFPT_GET_CONFIG_REPLY: switch configuration reply (switch -> controller)");
			break;

		case OFPT_SET_CONFIG:
			return string("OFPT_SET_CONFIG: set switch configuration (controller -> switch)");
			break;

		case OFPT_PACKET_IN:
			return string("OFPT_PACKET_IN: raw packet input (switch -> controller)");
			break;

		case OFPT_FLOW_REMOVED:
			return string("OFPT_FLOW_REMOVED: flow evicted (switch -> controller)");
			break;

		case OFPT_PORT_STATUS:
			return string("OFPT_PORT_STATUS: switch port notification (switch -> controller)");
			break;

		case OFPT_PACKET_OUT:
			return string("OFPT_PACKET_OUT: raw packet output (controller -> switch)");
			break;

		case OFPT_FLOW_MOD:
			return string("OFPT_FLOW_MOD: flow updated (controller -> switch)");
			break;

		case OFPT_PORT_MOD:
			return string("OFPT_PORT_MOD: port state updated (switch -> controller)");
			break;

		case OFPT_STATS_REQUEST:
			return string("OFPT_STATS_REQUEST: operational statistics request (controller -> switch)");
			break;

		case OFPT_STATS_REPLY:
			return string("OFPT_STATS_REPLY: operational statistics reply (switch -> controller)");
			break;

		case OFPT_BARRIER_REQUEST:
			return string("OFPT_BARRIER_REQUEST: operation fence (controller -> switch)");
			break;

		case OFPT_BARRIER_REPLY:
			return string("OFPT_BARRIER_REPLY: prior operations completed (switch -> controller");
			break;

		case OFPT_QUEUE_GET_CONFIG_REQUEST:
			return string("OFPT_QUEUE_GET_CONFIG_REQUEST: QoS configuration request (controller -> switch)");
			break;

		case OFPT_QUEUE_GET_CONFIG_REPLY:
			return string("OFPT_QUEUE_GET_CONFIG_REPLY: QoS configuration reply (switch -> controller)");
			break;
	}

	return string("(unknown openflow message type?)");
}


// serialize method for serializable class
uint32_t of_message::serialize(autobuf& dest) const {
	dest.clear();
	dest.create_empty_buffer(sizeof(struct ofp_header), false);
	struct ofp_header* header = (struct ofp_header*)
		dest.get_content_ptr_mutable();

	header->version = version;
	header->type = (uint8_t) msg_type;
	header->length = htons(msg_size);
	header->xid = htonl(xid);

	return sizeof(struct ofp_header);
}

// deserialize method for serializable class
bool of_message::deserialize(const autobuf& input) {
	const struct ofp_header* hdr = (const struct ofp_header*)
		input.get_content_ptr();
	clear();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_header))
		goto fail;

	// check for message type
	if (hdr->type > (int) OFPT_QUEUE_GET_CONFIG_REPLY)
		goto fail;
	#endif

	// set message variables
	version = hdr->version;
	msg_type = (enum ofp_type) hdr->type;
	msg_size = ntohs(hdr->length);
	xid = ntohl(hdr->xid);

	// deserialization OK
	return true;

fail:
	clear();
	return false;
}

// returns verbose base message type
string of_message::to_string() const {
	char buf[512];
	string m_type = of_common::msg_type_to_string(msg_type);

	sprintf(buf, "v%d | %s | len %d | xid %d",
		version, m_type.c_str(), msg_size, xid);

	return string(buf);
}
