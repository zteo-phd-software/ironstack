#include "of_message_port_modification.h"
using namespace std;

// constructor
of_message_port_modification::of_message_port_modification() {
	clear();
}

// clears the object
void of_message_port_modification::clear() {
	of_message::clear();
	msg_type = OFPT_PORT_MOD;

	port_no = (uint16_t) OFPP_NONE;
	port_mac_address.clear();

	new_config.clear();
	change_features = false;
	features.clear();
}

// generates a readable form of the message
string of_message_port_modification::to_string() const {
	char buf[128];
	std::string result = of_message::to_string();
	result += "\nport: ";
	sprintf(buf, "%u", port_no);
	result += buf;
	result += "\nmac address: ";
	result += port_mac_address.to_string();
	result += "\nnew configuration: ";
	result += new_config.to_string();
	result += "\nchange features: ";
	result += (change_features ? "YES" : "NO");

	if (change_features) {
		result += "\nnew features: ";
		result += features.to_string();
	}

	return result;
}

// serializes the message
uint32_t of_message_port_modification::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_port_mod), false);
	struct ofp_port_mod* hdr = (struct ofp_port_mod*) dest.get_content_ptr_mutable();

	hdr->header.version = version;
	hdr->header.type = (uint8_t) msg_type;
	hdr->header.length = htons(sizeof(struct ofp_port_mod));
	hdr->header.xid = htonl(xid);

	uint32_t new_config_val = new_config.get();
	uint32_t mask = 0xffffffff;

	hdr->port_no = htons(port_no);
	port_mac_address.get(hdr->hw_addr);

	hdr->config = htonl(new_config_val);
	hdr->mask = htonl(mask);

	hdr->advertise = (change_features ? htonl(features.get()) : 0);

	return sizeof(struct ofp_port_mod);
}

// should not be called
bool of_message_port_modification::deserialize(const autobuf& input) {
	abort();
	return false;
}
