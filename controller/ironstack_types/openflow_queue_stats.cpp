#include "openflow_queue_stats.h"

// constructor
openflow_queue_stats::openflow_queue_stats() {
	clear();
}

// clears the object
void openflow_queue_stats::clear() {
	all_ports = false;
	port = 0;
	all_queues = false;
	queue_id = 0;
	tx_bytes = 0;
	tx_bytes = 0;
	tx_errors = 0;
}

// generates a readable version of the object
string openflow_queue_stats::to_string() const {
	char buf[32];
	string result;
	sprintf(buf, "%hu", port);
	result += string("\nport  : ") + string(all_ports ? "ALL PORTS" : buf);

	sprintf(buf, "%u", queue_id);
	result += string("\nqueue : ") + string(all_queues ? "ALL QUEUES" : buf);

	sprintf(buf, "%" PRIu64, tx_packets);
	result += string("\ntx packets : ") + string(buf);

	sprintf(buf, "%" PRIu64 , tx_bytes);
	result += string("\ntx bytes   : ") + string(buf);

	sprintf(buf, "%" PRIu64, tx_errors);
	result += string("\ntx errors  : ") + string(buf);

	return result;
}

// serializes the object
uint32_t openflow_queue_stats::serialize(autobuf& dest) const {
	
	dest.create_empty_buffer(sizeof(struct ofp_queue_stats), true);
	struct ofp_queue_stats* ptr = (struct ofp_queue_stats*)
		dest.get_content_ptr_mutable();

	ptr->port_no = htons(all_ports ? (uint16_t) OFPP_ALL : port);
	ptr->queue_id = htonl(all_queues ? (uint32_t) OFPQ_ALL : queue_id);
	ptr->tx_bytes = htobe64(tx_bytes);
	ptr->tx_packets = htobe64(tx_packets);
	ptr->tx_errors = htobe64(tx_errors);

	return dest.size();
}

// deserializes the object
bool openflow_queue_stats::deserialize(const autobuf& input) {
	clear();

	const struct ofp_queue_stats* ptr = (const struct ofp_queue_stats*)
		input.get_content_ptr();
	
	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_queue_stats)) {
		return false;
	}
	#endif

	port = ntohs(ptr->port_no);
	if (port == (uint16_t) OFPP_ALL) {
		all_ports = true;
	}

	queue_id = ntohl(ptr->queue_id);
	if (queue_id == (uint32_t) OFPQ_ALL) {
		all_queues = true;
	}

	tx_bytes = be64toh(ptr->tx_bytes);
	tx_packets = be64toh(ptr->tx_packets);
	tx_errors = be64toh(ptr->tx_errors);

	return true;
}

