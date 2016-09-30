#include "of_port_features.h"

// default constructor
of_port_features::of_port_features() {
	clear();
}

// shortcut constructor
of_port_features::of_port_features(uint32_t raw_value) {
	set(raw_value);
}

// clears the object
void of_port_features::clear() {
	bandwidth_10mb_half_duplex = false;
	bandwidth_10mb_full_duplex = false;
	bandwidth_100mb_half_duplex = false;
	bandwidth_100mb_full_duplex = false;
	bandwidth_1gb_half_duplex = false;
	bandwidth_1gb_full_duplex = false;
	bandwidth_10gb_full_duplex = false;
	medium_copper = false;
	medium_fiber = false;
	auto_negotiation = false;
	pause = false;
	asymmetric_pause = false;
}

// sets the state based on an input raw value
void of_port_features::set(uint32_t raw_value) {
	bandwidth_10mb_half_duplex = (raw_value & (uint32_t) OFPPF_10MB_HD) ? true : false;
	bandwidth_10mb_full_duplex = (raw_value & (uint32_t) OFPPF_10MB_FD) ? true : false;
	bandwidth_100mb_half_duplex = (raw_value & (uint32_t) OFPPF_100MB_HD) ? true : false;
	bandwidth_100mb_full_duplex = (raw_value & (uint32_t) OFPPF_100MB_FD) ? true : false;
	bandwidth_1gb_half_duplex = (raw_value & (uint32_t) OFPPF_1GB_HD) ? true : false;
	bandwidth_1gb_full_duplex = (raw_value & (uint32_t) OFPPF_1GB_FD) ? true : false;
	bandwidth_10gb_full_duplex = (raw_value & (uint32_t) OFPPF_10GB_FD) ? true : false;
	medium_copper = (raw_value & (uint32_t) OFPPF_COPPER) ? true : false;
	medium_fiber = (raw_value & (uint32_t) OFPPF_FIBER) ? true : false;
	auto_negotiation = (raw_value & (uint32_t) OFPPF_AUTONEG) ? true : false;
	pause = (raw_value & (uint32_t) OFPPF_PAUSE) ? true : false;
	asymmetric_pause = (raw_value & (uint32_t) OFPPF_PAUSE_ASYM) ? true : false;
}

// returns the raw value derived from the state
uint32_t of_port_features::get() const {
	uint32_t result = 0;

	result |= (bandwidth_10mb_half_duplex ? (uint32_t) OFPPF_10MB_HD : 0);
	result |= (bandwidth_10mb_full_duplex ? (uint32_t) OFPPF_10MB_FD : 0);
	result |= (bandwidth_100mb_half_duplex ? (uint32_t) OFPPF_100MB_HD : 0);
	result |= (bandwidth_100mb_full_duplex ? (uint32_t) OFPPF_100MB_FD : 0);
	result |= (bandwidth_1gb_half_duplex ? (uint32_t) OFPPF_1GB_HD : 0);
	result |= (bandwidth_1gb_full_duplex ? (uint32_t) OFPPF_1GB_FD : 0);
	result |= (bandwidth_10gb_full_duplex ? (uint32_t) OFPPF_10GB_FD : 0);
	result |= (medium_copper ? (uint32_t) OFPPF_COPPER : 0);
	result |= (medium_fiber ? (uint32_t) OFPPF_FIBER : 0);
	result |= (auto_negotiation ? (uint32_t) OFPPF_AUTONEG : 0);
	result |= (pause ? (uint32_t) OFPPF_PAUSE : 0);
	result |= (asymmetric_pause ? (uint32_t) OFPPF_PAUSE_ASYM : 0);

	return result;
}

// generates a debug string about the features in this port
std::string of_port_features::to_string() const {
	std::string result;
	if (bandwidth_10mb_half_duplex || bandwidth_10mb_full_duplex) {
		result += "10mbit: ";

		if (bandwidth_10mb_half_duplex) {
			result += "HD ";
		}

		if (bandwidth_10mb_full_duplex) {
			result += "FD ";
		}

		result += "| ";
	}

	if (bandwidth_100mb_half_duplex || bandwidth_100mb_full_duplex) {
		result += "100mbit: ";
		if (bandwidth_100mb_half_duplex) {
			result += "HD ";
		}

		if (bandwidth_100mb_full_duplex) {
			result += "FD ";
		}

		result += "| ";
	}	

	if (bandwidth_1gb_half_duplex || bandwidth_1gb_full_duplex) {
		result += "1gbit: ";
		if (bandwidth_1gb_half_duplex) {
			result += "HD ";
		}

		if (bandwidth_1gb_full_duplex) {
			result += "FD ";
		}

		result += "| ";
	}

	if (bandwidth_10gb_full_duplex) {
		result += "10 gbit: FD | ";
	}
	
	result += "medium: ";
	if (medium_copper || medium_fiber) {
		if (medium_copper) {
			result += "copper ";
		}

		if (medium_fiber) {
			result += "fiber ";
		}
	}	else {
		result += "not specified ";
	}
	result += "| ";

	result += "auto_negotiate: ";
	if (auto_negotiation) {
		result += "yes | ";
	} else {
		result += "no | ";
	}

	result += "pause: ";
	if (pause) {
		result += "yes | ";
	} else {
		result += "no | ";
	}
	
	result += "asy_pause: ";
	if (asymmetric_pause) {
		result += "yes";
	} else {
		result += "no";
	}

	return result;
}
