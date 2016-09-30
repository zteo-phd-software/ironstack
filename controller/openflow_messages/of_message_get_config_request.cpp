#include "of_message_get_config_request.h"

// constructor
of_message_get_config_request::of_message_get_config_request() {
	clear();
}

// clears the message contents
void of_message_get_config_request::clear() {
	of_message::clear();
	msg_type = OFPT_GET_CONFIG_REQUEST;
}

// returns verbose information about this message type
std::string of_message_get_config_request::to_string() const {
	return of_message::to_string();
}

// serializes the message into a transmissible form
uint32_t of_message_get_config_request::serialize(autobuf& dest) const {
	return of_message::serialize(dest);
}

// should never need to get called -- switch never replies with this message
// type
bool of_message_get_config_request::deserialize(const autobuf& input) {
	abort();
	return false;
}
