#pragma once
#include <string>
#include <arpa/inet.h>
#include <inttypes.h>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
using namespace std;

// externally usable class
class openflow_port_stats {
public:

	// constructor
	openflow_port_stats();

	// clears the object
  void clear();

	// generates debug string information
	string to_string() const;

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
	bool      all_ports;
	uint16_t	port;

	bool      rx_packets_supported;
	uint64_t  rx_packets;

	bool      tx_packets_supported;
	uint64_t  tx_packets;

	bool      rx_bytes_supported;
	uint64_t  rx_bytes;

	bool      tx_bytes_supported;
	uint64_t  tx_bytes;

	bool      rx_dropped_supported;
	uint64_t  rx_dropped;

	bool      tx_dropped_supported;
	uint64_t  tx_dropped;

	bool      rx_errors_supported;
	uint64_t  rx_errors;

	bool      tx_errors_supported;
	uint64_t  tx_errors;

	bool      rx_frame_errors_supported;
	uint64_t  rx_frame_errors;

	bool      rx_overrun_errors_supported;
	uint64_t  rx_overrun_errors;

	bool      rx_crc_errors_supported;
	uint64_t  rx_crc_errors;

	bool      collision_errors_supported;
	uint64_t  collision_errors;

};
