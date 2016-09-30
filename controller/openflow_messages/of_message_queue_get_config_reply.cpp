#include "of_message_queue_get_config_reply.h"
#include "../gui/output.h"
using namespace std;

// constructor
of_message_queue_get_config_reply::of_message_queue_get_config_reply() {
	clear();
}

// clears the object
void of_message_queue_get_config_reply::clear() {
	of_message::clear();
	msg_type = OFPT_QUEUE_GET_CONFIG_REPLY;
	port = 0;
	queues.clear();
}

// generates a readable debug output for this message
string of_message_queue_get_config_reply::to_string() const {
	char buf[16];
	string result = of_message::to_string();
	result += "\nport: " + of_common::port_to_string(port);
	sprintf(buf, "%u", (uint32_t) queues.size());
	result += "\ntotal queues: ";
	result += buf;

	if (queues.size() > 0) {
		result += "\nqueue information:\n---------------------------------------\n";
		for (uint16_t counter = 0; counter < queues.size()-1; counter++) {
			result += queues[counter].to_string() + "\n";
		}
		result += queues[queues.size()-1].to_string();
	}
	
	return result;
}

// should never be called
uint32_t of_message_queue_get_config_reply::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the message
bool of_message_queue_get_config_reply::deserialize(const autobuf& input) {
	clear();

	output::log(output::loglevel::BUG, "of_message_queue_get_config_reply::deserialize() error -- incomplete implementation (review codebase).\n");
	abort();
	return false;

	/* TODO -- also need to fix up of_queue_config deserialization

	const struct ofp_queue_get_config_reply* hdr = (const struct ofp_queue_get_config_reply*) input.get_content_ptr();

	uint16_t bytes_used;
	uint16_t n_queues;
	autobuf configurations;
	of_queue_config current_config;

	bool status = of_message::deserialize(input);

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || msg_type != OFPT_QUEUE_GET_CONFIG_REPLY || input.size() < sizeof(struct ofp_queue_get_config_reply)) {
		goto fail;
	}
	#endif
	
	port = ntohs(hdr->port);
	
	// enter loop to process each configuration
	configurations.inherit_read_only(input.ptr_offset(sizeof(struct ofp_queue_get_config_reply)), input.size() - sizeof(struct ofp_queue_get_config_reply));

	// count number of queues
	n_queues = of_queue_config::count_queues(configurations);
	if (n_queues == 0) {
		return true;
	}

	// deserialize all queue information
	queues.reserve(n_queues);
	for (uint16_t counter = 0; counter < n_queues; counter++) {
		if (!current_config.deserialize(configurations, &bytes_used)) {
			goto fail;
		}
		queues.push_back(current_config);
		configurations.trim_front(bytes_used);
	}

	return true;

fail:
	clear();
	return false;
	*/
}
