#pragma once

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <endian.h>
#include <inttypes.h>
#include "of_message.h"
#include "../openflow_types/of_match.h"
#include "../utils/openflow_utils.h"

using namespace std;
class of_message_flow_removed : public of_message {
public:

	of_message_flow_removed();
	virtual ~of_message_flow_removed() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	of_match  match;
	uint64_t  cookie;
	uint16_t  priority;
	
	bool      reason_idle_timeout;
	bool      reason_hard_timeout;
	bool      reason_manually_removed;

	uint32_t  duration_alive_sec;
	uint32_t  duration_alive_nsec;
	uint16_t  original_idle_timeout;

	uint64_t  packet_count;
	uint64_t  byte_count;

	// serialization and deserialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
