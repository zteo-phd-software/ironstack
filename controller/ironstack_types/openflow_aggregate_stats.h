#pragma once
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <string>
#include <inttypes.h>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
using namespace std;

// externally usable class
class openflow_aggregate_stats {
public:

	// constructor
	openflow_aggregate_stats():packet_count(0),
		byte_count(0),
		flow_count(0) {}

	// clears the object
  void clear() { 
		packet_count = 0;
		byte_count = 0;
		flow_count = 0;
  }

	// generates debug string information
	string to_string() const { 
		string result = string("--- aggregate statistics ---\n");
		char buf[32];
		sprintf(buf, "%" PRIu64, packet_count);
		result += string("\ntotal packets : ") + string(buf);
		sprintf(buf, "%" PRIu64, byte_count);
		result += string("\ntotal bytes   : ") + string(buf);
		sprintf(buf, "%" PRIu64, flow_count);
		result += string("\ntotal flows   : ") + string(buf);
		return result;
	}

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
	uint64_t packet_count;
	uint64_t byte_count;
	uint64_t flow_count;

};
