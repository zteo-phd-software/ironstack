#include "of_message_echo_request.h"

using namespace std;
bool of_message_echo_request::display_content = false;

// constructor
of_message_echo_request::of_message_echo_request() {
	clear();
}

// clears the object
void of_message_echo_request::clear() {
	of_message::clear();
	msg_type = OFPT_ECHO_REQUEST;
	data.clear();
}

// generates a user-readable string
string of_message_echo_request::to_string() const {

	string result = of_message::to_string() + "\n";
	if (!display_content) {
		result += "payload content display suppressed.";
	}	else {
		result += "\npayload contents:\n";
		result += data.to_hex();
	}

	return result;
}

// serializes the message into a transmissible form
uint32_t of_message_echo_request::serialize(autobuf& dest) const {

	uint32_t size_required = sizeof(struct ofp_header) + data.size();

	dest.clear();
	dest.create_empty_buffer(size_required, false);

	struct ofp_header* hdr = (struct ofp_header*) dest.get_content_ptr_mutable();
	hdr->version = version;
	hdr->type = (uint8_t) msg_type;
	hdr->length = htons(size_required);
	hdr->xid = htonl(xid);
	if (data.size() > 0) {
		memcpy(dest.ptr_offset_mutable(sizeof(struct ofp_header)), data.get_content_ptr(), data.size());
	}

	return size_required;
}

// deserializes the echo message
bool of_message_echo_request::deserialize(const autobuf& input) {
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

// toggles display of content
void of_message_echo_request::show_contents(bool state) {
	display_content = state;
}
