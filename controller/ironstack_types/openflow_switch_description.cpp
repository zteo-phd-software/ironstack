#include "openflow_switch_description.h"
using namespace std;

// serializes the switch description object (back into the native
// struct ofp_desc_stats)
uint32_t openflow_switch_description::serialize(autobuf& dest) const {
	
	dest.create_empty_buffer(sizeof(struct ofp_desc_stats), true);
	struct ofp_desc_stats* ptr = (struct ofp_desc_stats*)
		dest.get_content_ptr_mutable();
	strncpy(ptr->mfr_desc, manufacturer.c_str(), DESC_STR_LEN-1);
	strncpy(ptr->hw_desc, hardware_description.c_str(), DESC_STR_LEN-1);
	strncpy(ptr->sw_desc, software_description.c_str(), DESC_STR_LEN-1);
	strncpy(ptr->serial_num, serial_number.c_str(), SERIAL_NUM_LEN-1);
	strncpy(ptr->dp_desc, general_description.c_str(), DESC_STR_LEN-1);

	return dest.size();
}

// deserializes the switch description object
bool openflow_switch_description::deserialize(const autobuf& input) {

	clear();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (input.size() < sizeof(struct ofp_desc_stats)) {
		return false;
	}
	#endif

	char desc_buf[DESC_STR_LEN];
	char serial_buf[SERIAL_NUM_LEN];
	memset(desc_buf, 0, sizeof(desc_buf));
	memset(serial_buf, 0, sizeof(serial_buf));

	const struct ofp_desc_stats* ptr = (const struct ofp_desc_stats*)
		input.get_content_ptr();

	strncpy(desc_buf, ptr->mfr_desc, DESC_STR_LEN-1);
	manufacturer = string((const char*)desc_buf);
	strncpy(desc_buf, ptr->hw_desc, DESC_STR_LEN-1);
	hardware_description = string((const char*)desc_buf);
	strncpy(desc_buf, ptr->sw_desc, DESC_STR_LEN-1);
	software_description = string((const char*)desc_buf);
	strncpy(serial_buf, ptr->serial_num, SERIAL_NUM_LEN-1);
	serial_number = string((const char*)serial_buf);
	strncpy(desc_buf, ptr->dp_desc, DESC_STR_LEN-1);
	general_description = string((const char*)desc_buf);

	return true;
}
