#include "of_queue_config.h"

// constructor
of_queue_config::of_queue_config() {
	clear();
}

// shortcut constructor
of_queue_config::of_queue_config(const autobuf& input) {
	uint16_t dummy;
	deserialize(input, &dummy);
}

// clears the object
void of_queue_config::clear() {
	queue_id = 0;
	has_property = false;
	min_rate_enabled = false;
	min_rate = 0;
}

// sets the property for this object
// since only one type of property is available in openflow 1.0, we operate with this assumption
bool of_queue_config::deserialize(const autobuf& input, uint16_t* bytes_used) {
	struct ofp_packet_queue header;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	memset(&header, 0, sizeof(header));
	if (input.size() < sizeof(struct ofp_packet_queue))
	{
		*bytes_used = 0;
		return false;
	}
	#endif

	memcpy(&header, input.get_content_ptr(), sizeof(struct ofp_packet_queue));

	uint16_t len = ntohs(header.len);
	queue_id = ntohl(header.queue_id);

	if (len == 0) {
		has_property = false;
		min_rate_enabled = false;
		min_rate = 0;

		*bytes_used = (uint16_t) sizeof(struct ofp_packet_queue);
	} else {
		// extract property
		autobuf properties(((const char*)input.get_content_ptr())+sizeof(struct ofp_packet_queue), len);
		assert(properties.size() == sizeof(struct ofp_queue_prop_min_rate));

		struct ofp_queue_prop_min_rate rate;
		memcpy(&rate, properties.get_content_ptr(), sizeof(struct ofp_queue_prop_min_rate));
		rate.prop_header.property = ntohs(rate.prop_header.property);
		assert(rate.prop_header.property == (uint16_t) OFPQT_MIN_RATE);

		rate.rate = ntohs(rate.rate);
		if (rate.rate > 1000) {
			min_rate_enabled = false;
			min_rate = 0;
		} else {
			min_rate = rate.rate;
		}

		*bytes_used = (uint16_t)(sizeof(struct ofp_packet_queue)+sizeof(struct ofp_queue_prop_min_rate));
	}

	return true;
}

// counts the number of queue objects in this packed list
uint16_t of_queue_config::count_queues(const autobuf& input) {
	autobuf composite = input;
	struct ofp_packet_queue header;
	uint16_t len = 0;
	uint16_t result = 0;

	memset(&header, 0, sizeof(header));
	while (composite.size() > sizeof(header)) {
		memcpy(&header, composite.get_content_ptr(), sizeof(header));
		len = ntohs(header.len);

		// TODO -- not sure what this len should be: is it sizeof(struct ofp_packet_queue) + all queue descriptions or just size of all queue descriptions?
		// this code assumes the latter
		composite.trim_front(sizeof(struct ofp_packet_queue) + len);
		result++;
	}
	
	// if the assumption above is correct then this should hold true
	assert(composite.size() == 0);

	return result;
}

// generates a user-readable message
std::string of_queue_config::to_string() const {
	char buf[16];
	std::string result = "queue id: ";
	sprintf(buf, "%u", queue_id);
	result += buf;
	
	if (!has_property) {
		result += "\nno additional queue properties.";
	} else {
		if (!min_rate_enabled) {
			result += "\nminimum rate disabled.";
		}	else {
			result += "\nminimum rate set to: ";
			sprintf(buf, "%0.2f", (float) min_rate/10.0);
			result += buf;
			result += "%%";
		}
	}

	return result;
}
