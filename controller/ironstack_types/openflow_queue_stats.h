#pragma once
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <arpa/inet.h>
#include <inttypes.h>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
#include <string>
using namespace std;

// externally usable class
class openflow_queue_stats {
public:

	// constructor
	openflow_queue_stats();

	// clears the object
  void clear();

	// generates debug string information
	string to_string() const;

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
	bool all_ports;
	uint16_t port;

	bool all_queues;
	uint32_t queue_id;

	uint64_t tx_bytes;
	uint64_t tx_packets;
	uint64_t tx_errors;
};
