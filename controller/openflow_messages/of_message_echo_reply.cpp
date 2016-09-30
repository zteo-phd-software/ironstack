#include "of_message_echo_reply.h"

using namespace std;

// constructior
of_message_echo_reply::of_message_echo_reply() {
	clear();
}

// clears the object
void of_message_echo_reply::clear() {
	of_message::clear();
	msg_type = OFPT_ECHO_REPLY;
	data.reset();
}

// generates a user-readable string
string of_message_echo_reply::to_string() const {
	string result = of_message::to_string();
	if (data.size() > 0) {
		result += string("data contents:\n");
		result += data.to_hex();
	} else {
		result += string("\nno data content.");
	}
	return result;
}

// serializes the message into a transmissible form
uint32_t of_message_echo_reply::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_header) + data.size();

	dest.clear();
	dest.create_empty_buffer(size_required, false);

	struct ofp_header* hdr = (struct ofp_header*) dest.get_content_ptr_mutable();
	hdr->version = version;
	hdr->type = (uint8_t) msg_type;
	hdr->length = htons(size_required);
	hdr->xid = htonl(xid);
	if (data.size() > 0) {
		memcpy(dest.ptr_offset_mutable(sizeof(struct ofp_header)),
			data.get_content_ptr(), data.size());
	}

	return size_required;
}

// deserializes the echo message
bool of_message_echo_reply::deserialize(const autobuf& input) {
	clear();

	uint32_t payload_len;
	bool result = of_message::deserialize(input);
	if (!result) {
		goto fail;
	}

	payload_len = msg_size - sizeof(struct ofp_header);
	input.read_rear(data, payload_len);
	
	return true;

fail:

	clear();
	return false;
}

