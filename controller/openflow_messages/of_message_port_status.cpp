#include "of_message_port_status.h"

// default constructor
of_message_port_status::of_message_port_status() {
	clear();
}

// clears the message
void of_message_port_status::clear() {
	of_message::clear();
	msg_type = OFPT_PORT_STATUS;
	msg_size = sizeof(struct ofp_port_status);
	reason = OFPPR_MODIFY;
}

// generates a readable string
std::string of_message_port_status::to_string() const {
	std::string result = of_message::to_string() + "\nreason: ";

	switch (reason) {
		case (OFPPR_ADD):
			result += "PORT ADDED";
			break;

		case (OFPPR_DELETE):
			result += "PORT DELETED";
			break;

		case (OFPPR_MODIFY):
			result += "PORT MODIFIED";
			break;

		default:
			result += "UNKNOWN";
			break;
	}

	result += "\n" + description.to_string();
	return result;
}

// serialize method for serializable class
uint32_t of_message_port_status::serialize(autobuf& dest) const {
	// should never be called
	abort();
	return 0;
}

// deserializes the port modification message
bool of_message_port_status::deserialize(const autobuf& input) {

	clear();
	const struct ofp_port_status* hdr = (const struct ofp_port_status*) input.get_content_ptr();
	bool status = of_message::deserialize(input);

	#ifndef __NO_OPENFLOW_SAFTEY_CHECKS
	if (!status || 
		msg_type != OFPT_PORT_STATUS || 
		msg_size != sizeof(struct ofp_port_status)) {
		goto fail;
	}
	#endif

	switch (hdr->reason) {
		case ((uint8_t) OFPPR_ADD):
			reason = OFPPR_ADD;
			break;
		case ((uint8_t) OFPPR_DELETE):
			reason = OFPPR_DELETE;
			break;
		case ((uint8_t) OFPPR_MODIFY):
			reason = OFPPR_MODIFY;
			break;
		default:
			reason = OFPPR_MODIFY;
	}

	description.set(hdr->desc);
	return true;

fail:
	clear();
	return false;
}
