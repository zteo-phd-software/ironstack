#pragma once
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <string>
#include <inttypes.h>
#include "openflow_flow_description.h"

using namespace std;
class openflow_flow_description_and_stats {
public:

	// constructor
	openflow_flow_description_and_stats():flow_description(),
		table_id(0),
		duration_alive_sec(0),
		duration_alive_nsec(0),
		duration_to_idle_timeout(0),
		duration_to_hard_timeout(0),
		packet_count(0),
		byte_count(0) {}

	// shortcut constructor
	openflow_flow_description_and_stats(const openflow_flow_description& description):
		flow_description(description),
		table_id(0),
		duration_alive_sec(0),
		duration_alive_nsec(0),
		duration_to_idle_timeout(0),
		duration_to_hard_timeout(0),
		packet_count(0),
		byte_count(0) {}

	// clears the object
	void clear() {
		flow_description.clear();
		table_id = 0;
		duration_alive_sec = 0;
		duration_alive_nsec = 0;
		duration_to_idle_timeout = 0;
		duration_to_hard_timeout = 0;
		packet_count = 0;
		byte_count = 0;
	};

	// generates a printout of the information
	string to_string() const {
	  string result = flow_description.to_string();
		char buf[32];
		sprintf(buf, "%d", table_id);
		result += string("\ntable id        : ") + string(buf);

		sprintf(buf, "%u", duration_alive_sec);
		result += string("\nalive           : ") + string(buf) + string("s");

		sprintf(buf, "%u", duration_to_idle_timeout);
		result += string("\nto idle timeout : ") + string(buf) + string("s");

		sprintf(buf, "%u", duration_to_hard_timeout);
		result += string("\nto hard timeout : ") + string(buf) + string("s");

		sprintf(buf, "%" PRIu64, packet_count);
		result += string("\npacket count    : ") + string(buf);

		sprintf(buf, "%" PRIu64, byte_count);
		result += string("\nbyte count      : ") + string(buf);
		return result;
	};

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
	openflow_flow_description flow_description;
	uint8_t table_id;

	uint32_t duration_alive_sec;
	uint32_t duration_alive_nsec;

	uint16_t duration_to_idle_timeout;
	uint16_t duration_to_hard_timeout;

	uint64_t packet_count;
	uint64_t byte_count;

};
