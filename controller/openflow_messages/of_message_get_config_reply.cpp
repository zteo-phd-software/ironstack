#include "of_message_get_config_reply.h"
using namespace std;

// constructor
of_message_get_config_reply::of_message_get_config_reply() {
	clear();
}

// clears the object
void of_message_get_config_reply::clear() {
	of_message::clear();
	msg_type = OFPT_GET_CONFIG_REPLY;
	config.clear();
}

// converts the object into a verbose output
string of_message_get_config_reply::to_string() const {
	string result = of_message::to_string();
	result += "\nswitch configuration: ";
	result += config.to_string();

	return result;
}

// should never be called -- controllers don't send this message to switches
uint32_t of_message_get_config_reply::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the configuration reply message
bool of_message_get_config_reply::deserialize(const autobuf& input) {
	clear();
	bool status = of_message::deserialize(input);
	if (!status) {
		goto fail;
	}
	
	// sanity check
	if (msg_type != OFPT_GET_CONFIG_REPLY) {
		goto fail;
	}

	// defer to object for deserialization
	if (!config.deserialize(input)) {
		goto fail;
	}

	return true;

fail:
	clear();
	return false;
}
