#include "of_message_queue_get_config_request.h"
using namespace std;

// constructor
of_message_queue_get_config_request::of_message_queue_get_config_request() {
	clear();
}

// clears the message
void of_message_queue_get_config_request::clear() {
	of_message::clear();
	msg_type = OFPT_QUEUE_GET_CONFIG_REQUEST;
	msg_size = sizeof(struct ofp_queue_get_config_request);
	port = 0;
}

// generates a human-readable form of the message
string of_message_queue_get_config_request::to_string() const {
	char buf[16];
	string result = of_message::to_string() + "\nrequest for queue config on port ";
	sprintf(buf, "%u", port);
	result += buf;

	return result;
}

// generates a serialized version of this message
uint32_t of_message_queue_get_config_request::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_queue_get_config_request), false);
	struct ofp_queue_get_config_request* hdr = (struct ofp_queue_get_config_request*) dest.get_content_ptr_mutable();

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(sizeof(struct ofp_queue_get_config_request));
	hdr->header.xid = htonl(xid);
	hdr->port = ntohs(port);

	return sizeof(struct ofp_queue_get_config_request);
}

// should never be called
bool of_message_queue_get_config_request::deserialize(const autobuf& input) {
	abort();
	return false;
}
