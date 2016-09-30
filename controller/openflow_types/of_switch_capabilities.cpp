#include "of_switch_capabilities.h"

// default constructor
of_switch_capabilities::of_switch_capabilities() {
	clear();
}

// shortcut constructor
of_switch_capabilities::of_switch_capabilities(uint32_t raw_value) {
	set(raw_value);
}

// clears out the object
void of_switch_capabilities::clear() {
	switch_flow_stats_supported = false;
	switch_table_stats_supported = false;
	switch_port_stats_supported = false;
	switch_stp_supported = false;
	switch_ip_reassembly_supported = false;
	switch_queue_stats_supported = false;
	switch_arp_match_ip_supported = false;
}

// takes in the raw value and processes it
void of_switch_capabilities::set(uint32_t raw_value) {
	switch_flow_stats_supported = (raw_value & (uint32_t) OFPC_FLOW_STATS) ? true : false;
	switch_table_stats_supported = (raw_value & (uint32_t) OFPC_TABLE_STATS) ? true : false;
	switch_port_stats_supported = (raw_value & (uint32_t) OFPC_PORT_STATS) ? true : false;
	switch_stp_supported = (raw_value & (uint32_t) OFPC_STP) ? true : false;
	switch_ip_reassembly_supported = (raw_value & (uint32_t) OFPC_IP_REASM) ? true : false;
	switch_queue_stats_supported = (raw_value & (uint32_t) OFPC_QUEUE_STATS) ? true : false;
	switch_arp_match_ip_supported = (raw_value & (uint32_t) OFPC_ARP_MATCH_IP) ? true : false;
}

// returns the capability value in a uint32
uint32_t of_switch_capabilities::get() const {
	uint32_t result = 0;

	result |= (switch_flow_stats_supported ? (uint32_t) OFPC_FLOW_STATS : 0);
	result |= (switch_table_stats_supported ? (uint32_t) OFPC_TABLE_STATS : 0);
	result |= (switch_port_stats_supported ? (uint32_t) OFPC_PORT_STATS : 0);
	result |= (switch_stp_supported ? (uint32_t) OFPC_STP : 0);
	result |= (switch_ip_reassembly_supported ? (uint32_t) OFPC_IP_REASM : 0);
	result |= (switch_queue_stats_supported ? (uint32_t) OFPC_QUEUE_STATS : 0);
	result |= (switch_arp_match_ip_supported ? (uint32_t) OFPC_ARP_MATCH_IP : 0);

	return result;
}

// generates a debug string
std::string of_switch_capabilities::to_string() const {
	std::string result = "switch capabilities:\n";

	if (switch_flow_stats_supported) {
		result += "  [X] ";
	} else {
		result += "  [ ] ";
	}
	result += "flow stats";

	if (switch_table_stats_supported) {
		result += "  [X] ";
	} else {
		result += "  [ ] ";
	}
	result += "table stats\n";

	if (switch_port_stats_supported) {
		result += "  [X] ";
	} else {
		result += "  [ ] ";
	}
	result += "port stats";

	if (switch_queue_stats_supported) {
		result += "  [X] ";
	} else {
		result += "  [ ] ";
	}
	result += "queue stats\n";

	if (switch_stp_supported) {
		result += "  [X] ";
	} else {
		result += "  [ ] ";
	}
	result += "STP";

	if (switch_ip_reassembly_supported) {
		result += "         [X] ";
	} else {
		result += "         [ ] ";
	}
	result += "IP reassembly\n";


	if (switch_arp_match_ip_supported) {
		result += "  [X] ";
	}	else {
		result += "  [ ] ";
	}
	result += "ARP match IP\n";

	return result;
}
