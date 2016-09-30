#pragma once
#include <arpa/inet.h>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
#include <string>
using namespace std;

// externally usable class
class openflow_table_stats {
public:

	// constructor
	openflow_table_stats();

	// clears the object
  void clear();

	// generates debug string information
	string to_string() const;

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
	uint8_t		table_id;
	string name;

	// master switches
	bool		all_wildcards_supported;
	bool		no_wildcards_supported;
	
		// slave switches
		bool	wildcard_in_port_supported;
		bool	wildcard_ethernet_src_supported;
		bool	wildcard_ethernet_dest_supported;
		bool	wildcard_vlan_id_supported;
		bool	wildcard_vlan_pcp_supported;
		bool	wildcard_ethernet_frame_type_supported;
		bool	wildcard_ip_type_of_service_supported;
		bool	wildcard_ip_protocol_supported;
		uint8_t	wildcard_ip_src_lsb_supported;
		uint8_t	wildcard_ip_dest_lsb_supported;
		bool	wildcard_tcpudp_src_port_supported;
		bool	wildcard_tcpudp_dest_port_supported;
	
	uint32_t	max_entries_supported;
	uint32_t	active_entries;
	uint64_t	lookup_count;
	uint64_t	matched_count;
};
