#include "openflow_port_config.h"

// default constructor
openflow_port_config::openflow_port_config() {
	clear();
}

// shortcut constructor
openflow_port_config::openflow_port_config(uint32_t raw_value) {
	set(raw_value);
}

// clears the object
void openflow_port_config::clear() {
	port_down = false;
	port_stp_disabled = false;
	port_drop_all_pkts_except_stp = false;
	port_drop_stp_pkts = false;
	port_exclude_from_flooding = false;
	port_drop_all_pkts = false;
	port_pkt_in_events_disabled = false;
}

// sets the state of this object according to the input raw value
void openflow_port_config::set(uint32_t raw_value) {
	port_down = (raw_value & (uint32_t) OFPPC_PORT_DOWN ?
		true : false);
	port_stp_disabled = (raw_value & (uint32_t) OFPPC_NO_STP ?
		true : false);
	port_drop_all_pkts_except_stp = (raw_value & (uint32_t) OFPPC_NO_RECV ?
		true : false);
	port_drop_stp_pkts = (raw_value & (uint32_t) OFPPC_NO_RECV_STP ?
		true : false);
	port_exclude_from_flooding = (raw_value & (uint32_t) OFPPC_NO_FLOOD ?
		true : false);
	port_drop_all_pkts = (raw_value & (uint32_t) OFPPC_NO_FWD ?
		true : false);
	port_pkt_in_events_disabled = (raw_value & (uint32_t) OFPPC_NO_PACKET_IN ?
		true : false);
}

// converts port configuration into (host order) uint32
uint32_t openflow_port_config::get() const {
	uint32_t result = 0;
	result |= (port_down ? (uint32_t) OFPPC_PORT_DOWN : 0);
	result |= (port_stp_disabled ? (uint32_t) OFPPC_NO_STP : 0);
	result |= (port_drop_all_pkts_except_stp ? (uint32_t) OFPPC_NO_RECV : 0);
	result |= (port_drop_stp_pkts ? (uint32_t) OFPPC_NO_RECV_STP : 0);
	result |= (port_exclude_from_flooding ? (uint32_t) OFPPC_NO_FLOOD : 0);
	result |= (port_drop_all_pkts ? (uint32_t) OFPPC_NO_FWD : 0);
	result |= (port_pkt_in_events_disabled ? (uint32_t) OFPPC_NO_PACKET_IN : 0);

	return result;
}

// generates a readable version of this object
std::string openflow_port_config::to_string() const {
	std::string result = "port config: ";

	if (get() == 0)
		result += "NORMAL ";

	if (port_down)
		result += "ADMIN_DOWN ";
	
	if (port_stp_disabled)
		result += "STP_DISABLED ";
	
	if (port_drop_all_pkts_except_stp)
		result += "DROP_ALL_XCEPT_STP ";
	
	if (port_drop_stp_pkts)
		result += "DROP_STP ";
	
	if (port_exclude_from_flooding)
		result += "EXCLUDE_FROM_FLOODING ";
	
	if (port_drop_all_pkts)
		result += "DROP_ALL_PKTS ";
	
	if (port_pkt_in_events_disabled)
		result += "PKT_IN_EVENT_DISABLED ";

	return result;
}
