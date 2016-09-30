#pragma once
#include "../utils/of_common_utils.h"
#include "../../common/autobuf.h"
#include "../../common/common_utils_oop.h"
#include "../../common/ip_address.h"
#include "../../common/openflow.h"
#include "../../common/mac_address.h"
#include <string>
using namespace std;

// openflow match object
//
// author : Z. Teo (zteo@cs.cornell.edu)
// updated: 10/11/14 (to_string() change)
// compile with __NO_OPENFLOW_SAFETY_CHECKS to gain increased speed

class of_match {
public:
	of_match();
	of_match(const string& request);

	// comparators
	bool operator==(const of_match& other) const;
	bool operator!=(const of_match& other) const;

	// subset operator (returns true if this match criteria is a wildcard subset of the other input)
	bool operator<(const of_match& other) const;

	// resets the match criteria (sets all wildcards to false)
	void clear();
	void wildcard_all();
	void wildcard_none();

	// string functions 
	string to_string() const;
	bool from_string(const string& request);  // example: "ethernet_src = 00:00:00:00:00:00 in_port = 2"

	// specify wildcards
	bool			wildcard_in_port;								// syntax: in_port = * | in_port = [n]
	bool			wildcard_ethernet_src;					// syntax: dl_src = * | dl_src = [eth addr]
	bool			wildcard_ethernet_dest;					// syntax: dl_dest = * | dl_dest = [eth addr]
	bool			wildcard_vlan_id;								// syntax: vlan_id = * | vlan_id = [n]
	bool			wildcard_vlan_pcp;							// syntax: vlan_pcp = * | vlan_pcp = [n]
	bool			wildcard_ethernet_frame_type;		// syntax: ethertype = * | ethertype = [n]
	bool			wildcard_ip_type_of_service;		// syntax: ip_tos = * | ip_tos = [n]
	bool			wildcard_ip_protocol;						// syntax: ip_proto = * | ip_proto = [n]
	uint8_t		wildcard_ip_src_lsb_count;			// number of least-significant bits to wildcard in ip (max 32)
																						// syntax: nw_src_lsb_wildcard = [n]
	uint8_t		wildcard_ip_dest_lsb_count;			// syntax: nw_dest_lsb_wildceard = [n]
	bool			wildcard_tcpudp_src_port;				// syntax: src_port = * | src_port = [n]
	bool			wildcard_tcpudp_dest_port;			// syntax: dest_port = * | dest_port = [n]

	// specify non-wildcarded fields
	uint16_t		in_port;
	mac_address ethernet_src;
	mac_address ethernet_dest;
	uint16_t		vlan_id;
	uint8_t			vlan_pcp;
	uint16_t		ethernet_frame_type;
	uint8_t			ip_type_of_service;
	uint8_t			ip_protocol;
	ip_address	ip_src_address;								// syntax: nw_src = * | nw_src = [ip addr]
	ip_address	ip_dest_address;							// syntax: nw_dest = * | nw_dest = [ip addr]
	uint16_t		tcpudp_src_port;
	uint16_t		tcpudp_dest_port;

	// serialization methods
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// (deprecated) generates the title for the rule set
//	static string generate_title();
};
