#include "of_message_packet_out.h"
using namespace std;

// constructor
of_message_packet_out::of_message_packet_out() {
	clear();
}

// clears the object
void of_message_packet_out::clear() {
	msg_type = OFPT_PACKET_OUT;
	buffer_id = 0xffffffff;
	use_port = false;
	in_port = 0;

	action_list.clear();
	packet_data.clear();
}

// generates a readable form of the message
string of_message_packet_out::to_string() const {
	char buf[128];
	string result = of_message::to_string();
	result += "\nbuffer id: ";
	sprintf(buf, "%u", buffer_id);
	result += buf;
	result += "\nin port: ";
	if (use_port) {
		sprintf(buf, "%u", in_port);
	} else {
		sprintf(buf, "NONE");
	}
	result += buf;
	result += action_list.to_string();

	return result;
}

// serializes the message
uint32_t of_message_packet_out::serialize(autobuf& dest) const {

	uint32_t action_list_len = action_list.get_serialization_size();
	uint32_t size_required = sizeof(struct ofp_packet_out) + action_list_len + packet_data.size();

	dest.clear();
	dest.create_empty_buffer(size_required, false);

	struct ofp_packet_out* hdr = (struct ofp_packet_out*) dest.get_content_ptr_mutable();
	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(size_required);
	hdr->header.xid = htonl(xid);

	hdr->buffer_id = htonl(buffer_id);
	if (use_port) {
		hdr->in_port = htons(in_port);
	} else {
		hdr->in_port = htons((uint16_t) OFPP_NONE);
	}

	hdr->actions_len = htons(action_list_len);
	autobuf action_buf;
	action_buf.inherit_shared(dest.ptr_offset_mutable(sizeof(struct ofp_packet_out)), action_list_len);
	action_list.serialize(action_buf);

	if (packet_data.size() > 0) {
		packet_data.memcpy_to(dest.ptr_offset_mutable(sizeof(struct ofp_packet_out) + action_list_len), packet_data.size());
//		memcpy((uint8_t*)dest.ptr_offset_const(sizeof(struct ofp_packet_out) + action_list_len), packet_data.get_content_ptr(), packet_data.size());
	}

	return size_required;
}

// should not be called
bool of_message_packet_out::deserialize(const autobuf& input) {
	abort();
	return false;
}
