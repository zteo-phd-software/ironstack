#include "of_port_state.h"

// default constructor
of_port_state::of_port_state() {
	clear();
}

// shortcut constructor
of_port_state::of_port_state(uint32_t raw_value) {
	set(raw_value);
}

// clears the object
void of_port_state::clear() {
	port_link_down = false;
	port_stp_listen = false;
	port_stp_learn = false;
	port_stp_forward = false;
	port_stp_block = false;
	port_stp_mask = false;
}

// sets the port state using a raw value
void of_port_state::set(uint32_t raw_value) {
	port_link_down = (raw_value & (uint32_t) OFPPS_LINK_DOWN ? true : false);
	port_stp_listen = (raw_value & (uint32_t) OFPPS_STP_LISTEN ? true : false);
	port_stp_learn = (raw_value & (uint32_t) OFPPS_STP_LEARN ? true : false);
	port_stp_forward = (raw_value & (uint32_t) OFPPS_STP_FORWARD ? true : false);
	port_stp_block = (raw_value & (uint32_t) OFPPS_STP_BLOCK ? true : false);
	port_stp_mask = (raw_value & (uint32_t) OFPPS_STP_MASK ? true : false);
}

uint32_t of_port_state::get() const {
	uint32_t result = 0;

	result |= (port_link_down ? (uint32_t) OFPPS_LINK_DOWN : 0);
	result |= (port_stp_listen ? (uint32_t) OFPPS_STP_LISTEN : 0);
	result |= (port_stp_learn ? (uint32_t) OFPPS_STP_LEARN : 0);
	result |= (port_stp_forward ? (uint32_t) OFPPS_STP_FORWARD : 0);
	result |= (port_stp_block ? (uint32_t) OFPPS_STP_BLOCK : 0);
	result |= (port_stp_mask ? (uint32_t) OFPPS_STP_MASK : 0);

	return result;
}

std::string of_port_state::to_string() const {
	std::string result = "port state: ";

	if (get() == 0) {
		result += "NORMAL ";
	}

	if (port_link_down) {
		result += "LINK_DOWN ";
	}
	
	if (port_stp_listen) {
		result += "STP_LISTEN ";
	}
	
	if (port_stp_learn) {
		result += "STP_LEARN ";
	}
	
	if (port_stp_forward) {
		result += "STP_FORWARD ";
	}
	
	if (port_stp_block) {
		result += "STP_BLOCK ";
	}
	
	if (port_stp_mask) {
		result += "STP_MASK ";
	}

	return result;
}
