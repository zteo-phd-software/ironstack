#include "of_message_error.h"

using namespace std;

// constructor
of_message_error::of_message_error() {
	clear();
}

// clears the object
void of_message_error::clear() {
	of_message::clear();
	msg_type = OFPT_ERROR;

	base_error.clear();
	extended_error.clear();
	error_data.clear();
}

// generates a readable form of this message
string of_message_error::to_string() const {
	string result = "error type: " + base_error + "\n";
	result += "extended error code: " + extended_error + "\n";

	if (base_error.compare("HELLO_FAILED") == 0) {
		result += "explanation: " + string(
			(const char*) error_data.get_content_ptr());
	}	else {
		result += string("original request details:\n");
		result += error_data.to_hex();
	}

	return result;
}

// should never be called
uint32_t of_message_error::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the error message and populates the data fields
bool of_message_error::deserialize(const autobuf& input) {
	struct ofp_error_msg header;
	uint16_t data_len = 0;

	clear();
	bool status = of_message::deserialize(input);

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status || input.size() < sizeof(struct ofp_error_msg)) {
		return false;
	}
	#endif

	data_len = msg_size - sizeof(struct ofp_error_msg);
	memcpy(&header, input.get_content_ptr(), sizeof(struct ofp_error_msg));
	header.type = ntohs(header.type);
	header.code = ntohs(header.code);

	switch (header.type) {
		case ((uint16_t) OFPET_HELLO_FAILED):
			base_error = "HELLO_FAILED";

			switch (header.code) {
				case ((uint16_t) OFPHFC_INCOMPATIBLE):
					extended_error = "NO_COMPATIBLE_VERSION";
					break;
				case ((uint16_t) OFPHFC_EPERM):
					extended_error = "PERMISSIONS_ERROR";
					break;
				default:
					extended_error = "UNKNOWN";
			}
			break;

		case ((uint16_t) OFPET_BAD_REQUEST):
			base_error = "BAD_REQUEST";

			switch (header.code) {
				case ((uint16_t) OFPBRC_BAD_VERSION):
					extended_error = "VERSION_NOT_SUPPORTED";
					break;
				case ((uint16_t) OFPBRC_BAD_TYPE):
					extended_error = "BAD_TYPE";
					break;
				case ((uint16_t) OFPBRC_BAD_STAT):
					extended_error = "STATS_REQUEST_TYPE_NOT_SUPPORTED";
					break;
				case ((uint16_t) OFPBRC_BAD_VENDOR):
					extended_error = "VENDOR_NOT_SUPPORTED";
					break;
				case ((uint16_t) OFPBRC_BAD_SUBTYPE):
					extended_error = "VENDOR_SUBTYPE_NOT_SUPPORTED";
					break;
				case ((uint16_t) OFPBRC_EPERM):
					extended_error = "PERMISSIONS_ERROR";
					break;
				case ((uint16_t) OFPBRC_BAD_LEN):
					extended_error = "BAD_LENGTH";
					break;
				case ((uint16_t) OFPBRC_BUFFER_EMPTY):
					extended_error = "BUFFER_ALREADY_USED";
					break;
				case ((uint16_t) OFPBRC_BUFFER_UNKNOWN):
					extended_error = "UNKNOWN_BUFFER";
					break;
				default:
					extended_error = "UNKNOWN";
			}
			break;

		case ((uint16_t) OFPET_BAD_ACTION):
			base_error = "BAD_ACTION";

			switch (header.code) {
				case ((uint16_t) OFPBAC_BAD_TYPE):
					extended_error = "BAD_TYPE";
					break;
				case ((uint16_t) OFPBAC_BAD_LEN):
					extended_error = "BAD_LENGTH";
					break;
				case ((uint16_t) OFPBAC_BAD_VENDOR):
					extended_error = "UNKNOWN_VENDOR_ID";
					break;
				case ((uint16_t) OFPBAC_BAD_VENDOR_TYPE):
					extended_error = "UNKNOWN_VENDOR_ACTION";
					break;
				case ((uint16_t) OFPBAC_BAD_OUT_PORT):
					extended_error = "INVALID_OUTPUT_PORT";
					break;
				case ((uint16_t) OFPBAC_BAD_ARGUMENT):
					extended_error = "BAD_ARGUMENT";
					break;
				case ((uint16_t) OFPBAC_EPERM):
					extended_error = "PERMISSIONS_ERROR";
					break;
				case ((uint16_t) OFPBAC_TOO_MANY):
					extended_error = "TOO_MANY_ACTIONS";
					break;
				case ((uint16_t) OFPBAC_BAD_QUEUE):
					extended_error = "INVALID_OUTPUT_QUEUE";
					break;
				default:
					extended_error = "UNKNOWN";
			}
			break;

		case ((uint16_t) OFPET_FLOW_MOD_FAILED):

			switch (header.code) {
				case ((uint16_t) OFPFMFC_ALL_TABLES_FULL):
					extended_error = "ALL_TABLES_FULL";
					break;
				case ((uint16_t) OFPFMFC_OVERLAP):
					extended_error = "OVERLAP_FLOW";
					break;
				case ((uint16_t) OFPFMFC_EPERM):
					extended_error = "PERMISSIONS_ERROR";
					break;
				case ((uint16_t) OFPFMFC_BAD_EMERG_TIMEOUT):
					extended_error = "NONZERO_IDLE_OR_HARD_TIMEOUT";
					break;
				case ((uint16_t) OFPFMFC_BAD_COMMAND):
					extended_error = "BAD_COMMAND";
					break;
				case ((uint16_t) OFPFMFC_UNSUPPORTED):
					extended_error = "UNSUPPORTED_ACTION_LIST";
					break;
				default:
					extended_error = "UNKNOWN";
			}

			break;

		case ((uint16_t) OFPET_PORT_MOD_FAILED):

			switch (header.code) {
				case ((uint16_t) OFPPMFC_BAD_PORT):
					extended_error = "BAD_PORT";
					break;
				case ((uint16_t) OFPPMFC_BAD_HW_ADDR):
					extended_error = "BAD_HARDWARE_ADDRESS";
					break;
				default:
					extended_error = "UNKNOWN";
			}

			break;

		case ((uint16_t) OFPET_QUEUE_OP_FAILED):

			switch (header.code) {
				case ((uint16_t) OFPQOFC_BAD_QUEUE):
					extended_error = "BAD_QUEUE";
					break;
				case ((uint16_t) OFPQOFC_EPERM):
					extended_error = "PERMISSIONS_ERROR";
					break;
				default:
					extended_error = "UNKNOWN";
			}

			break;

		default:

			base_error = "UNKNOWN";
			extended_error = "UNKNOWN";
	}

	if (data_len != 0) {
		error_data.set_content(((const char*)input.get_content_ptr()) +
			sizeof(struct ofp_error_msg), data_len);
	}

	return true;
}
