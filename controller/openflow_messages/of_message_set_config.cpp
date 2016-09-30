#include "of_message_set_config.h"
using namespace std;

// constructor
of_message_set_config::of_message_set_config() {
	clear();
}

// clears the object
void of_message_set_config::clear() {
	of_message::clear();
	msg_type = OFPT_SET_CONFIG;
	msg_size = sizeof(struct ofp_switch_config);

	frag_normal = false;
	frag_drop = false;
	frag_reasm = false;
	frag_mask = false;

	max_msg_send_len = 0;
}

// converts the object into a verbose output
string of_message_set_config::to_string() const {
	string result = of_message::to_string();
	result += "\nswitch configuration to set: ";

	if (frag_normal) {
		result += "FRAG_NORMAL ";
	} else if (frag_drop) {
		result += "FRAG_DROP ";
	} else if (frag_reasm) {
		result += "FRAG_REASM ";
	} else if (frag_mask) {
		result += "FRAG_MASK ";
	} else {
		result += "FRAG_NORMAL ";
	}
	result += "\n";

	result += "max message_in event summary: ";
	char buf[32];
	sprintf(buf, "%d bytes", max_msg_send_len);
	result += buf;

	return result;
}

// serializes the message for the switch
uint32_t of_message_set_config::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_switch_config), false);
	struct ofp_switch_config* hdr = (struct ofp_switch_config*) dest.get_content_ptr_mutable();

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(sizeof(struct ofp_switch_config));
	hdr->header.xid = htonl(xid);
	
	if (frag_normal) {
		hdr->flags = 0;
	} else if (frag_drop) {
		hdr->flags = 1;
	} else if (frag_reasm) {
		hdr->flags = 2;
	} else if (frag_mask) {
		hdr->flags = 3;
	} else {
		hdr->flags = 0;		// if nothing set, default to frag normal
	}
	hdr->flags = htons(hdr->flags);
	hdr->miss_send_len = htons(max_msg_send_len);

	return sizeof(struct ofp_switch_config);
}

// should never be called -- switches don't send this message
bool of_message_set_config::deserialize(const autobuf& input) {
	abort();
	return false;
}
