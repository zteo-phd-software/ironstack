#include "openflow_port_stats.h"
#include "../gui/output.h"

// constructor
openflow_port_stats::openflow_port_stats() {
	clear();
}

// clears the object
void openflow_port_stats::clear() {
	all_ports = false;
	port = 0;
	rx_packets_supported = false;
	rx_packets = 0;
	tx_packets_supported = false;
	tx_packets = 0;
	rx_bytes_supported = false;
	rx_bytes = 0;
	tx_bytes_supported = false;
	tx_bytes = 0;
	rx_dropped_supported = false;
	rx_dropped = 0;
	tx_dropped_supported = false;
	tx_dropped = 0;
	rx_errors_supported = false;
	rx_errors = 0;
	tx_errors_supported = false;
	tx_errors = 0;
	rx_frame_errors_supported = false;
	rx_frame_errors = 0;
	rx_overrun_errors_supported = false;
	rx_overrun_errors = 0;
	rx_crc_errors_supported = false;
	rx_crc_errors = 0;
	collision_errors_supported = false;
	collision_errors = 0;
}

// generates a readable version of the object
string openflow_port_stats::to_string() const {

	char buf[128];
	string result = string("\nport           : ");
	if (all_ports) {
		result += "ALL";
	} else {
		if (port == OFPP_CONTROLLER) {
			result += string("CONTROLLER");
		} else {
			sprintf(buf, "%u", port);
			result += buf;
		}
	}

	result += "\nrx packets     : ";
	if (rx_packets_supported) {
		sprintf(buf, "%" PRIu64, rx_packets);
		result += buf;
	} else {
		result += "not supported";
	}
	
	result += "\ntx packets     : ";
	if (tx_packets_supported) {
		sprintf(buf, "%" PRIu64, tx_packets);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx bytes       : ";
	if (rx_bytes_supported) {
		sprintf(buf, "%" PRIu64, rx_bytes);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\ntx bytes       : ";
	if (tx_bytes_supported) {
		sprintf(buf, "%" PRIu64, tx_bytes);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx dropped     : ";
	if (rx_dropped_supported) {
		sprintf(buf, "%" PRIu64, rx_dropped);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\ntx dropped     : ";
	if (tx_dropped_supported) {
		sprintf(buf, "%" PRIu64, tx_packets);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx errors      : ";
	if (rx_errors_supported) {
		sprintf(buf, "%" PRIu64, rx_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\ntx errors      : ";
	if (tx_errors_supported) {
		sprintf(buf, "%" PRIu64, tx_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx frame errors: ";
	if (rx_frame_errors_supported) {
		sprintf(buf, "%" PRIu64, rx_frame_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx overruns    : ";
	if (rx_overrun_errors_supported) {
		sprintf(buf, "%" PRIu64, rx_overrun_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\nrx crc errors  : ";
	if (rx_crc_errors_supported) {
		sprintf(buf, "%" PRIu64, rx_crc_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	result += "\ncollisions     : ";
	if (collision_errors_supported) {
		sprintf(buf, "%" PRIu64, collision_errors);
		result += buf;
	} else {
		result += "not supported";
	}

	return result;
}

// serializes the port stats
uint32_t openflow_port_stats::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(struct ofp_port_stats), true);
	struct ofp_port_stats* ptr = (struct ofp_port_stats*) dest.get_content_ptr_mutable();
	ptr->port_no = htons(all_ports ? (uint16_t) OFPP_NONE : port);
	ptr->rx_packets = htobe64(rx_packets_supported ? rx_packets : (uint64_t) -1);
	ptr->tx_packets = htobe64(tx_packets_supported ? tx_packets : (uint64_t) -1);
	ptr->rx_bytes = htobe64(rx_bytes_supported ? rx_bytes : (uint64_t) -1);
	ptr->tx_bytes = htobe64(rx_bytes_supported ? tx_bytes : (uint64_t) -1);
	ptr->rx_dropped = htobe64(rx_dropped_supported ? rx_dropped : (uint64_t) -1);
	ptr->tx_dropped = htobe64(tx_dropped_supported ? tx_dropped : (uint64_t) -1);
	ptr->rx_errors = htobe64(rx_errors_supported ? rx_errors : (uint64_t) -1);
	ptr->tx_errors = htobe64(tx_errors_supported ? tx_errors : (uint64_t) -1);
	ptr->rx_frame_err = htobe64(rx_frame_errors_supported ? rx_frame_errors : (uint64_t) -1);
	ptr->rx_over_err = htobe64(rx_overrun_errors_supported ? rx_overrun_errors : (uint64_t) -1);
	ptr->rx_crc_err = htobe64(rx_crc_errors_supported ? rx_crc_errors : (uint64_t) -1);
	ptr->collisions = htobe64(collision_errors_supported ? collision_errors : (uint64_t) -1);

	return dest.size();
}

// deserializes the port stats
bool openflow_port_stats::deserialize(const autobuf& input) {
	clear();

	const struct ofp_port_stats* ptr = (const struct ofp_port_stats*)
		input.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_port_stats)) {
		output::log(output::loglevel::BUG, "openflow_port_stats::deserialize() given size %u, need size %u\n", input.size(), (uint32_t) sizeof(struct ofp_port_stats));
		return false;
	}
	#endif

	port = ntohs(ptr->port_no);
	if (port == (uint16_t) OFPP_NONE) {
		all_ports = true;
	} else {
		all_ports = false;
	}

	rx_packets = be64toh(ptr->rx_packets);
	rx_packets_supported = (rx_packets == (uint64_t) -1) ? false : true;

	tx_packets = be64toh(ptr->tx_packets);
	tx_packets_supported = (tx_packets == (uint64_t) -1) ? false : true;

	rx_bytes = be64toh(ptr->rx_bytes);
	rx_bytes_supported = (rx_bytes == (uint64_t) -1) ? false : true;

	tx_bytes = be64toh(ptr->tx_bytes);
	tx_bytes_supported = (tx_bytes == (uint64_t) -1) ? false : true;

	rx_dropped = be64toh(ptr->rx_dropped);
	rx_dropped_supported = (rx_dropped == (uint64_t) -1) ? false : true;

	tx_dropped = be64toh(ptr->tx_dropped);
	tx_dropped_supported = (tx_dropped == (uint64_t) -1) ? false : true;

	rx_errors = be64toh(ptr->rx_errors);
	rx_errors_supported = (rx_errors == (uint64_t) -1) ? false : true;

	tx_errors = be64toh(ptr->tx_errors);
	tx_errors_supported = (tx_errors == (uint64_t) -1) ? false : true;

	rx_frame_errors = be64toh(ptr->rx_frame_err);
	rx_frame_errors_supported = (rx_frame_errors == (uint64_t) -1) ? false : true;

	rx_overrun_errors = be64toh(ptr->rx_over_err);
	rx_overrun_errors_supported = (rx_overrun_errors == (uint64_t) -1) ? false : true;

	rx_crc_errors = be64toh(ptr->rx_crc_err);
	rx_crc_errors_supported = (rx_crc_errors == (uint64_t) -1) ? false : true;

	collision_errors = be64toh(ptr->collisions);
	collision_errors_supported = (collision_errors == (uint64_t) -1) ? false : true;

	return true;
}
