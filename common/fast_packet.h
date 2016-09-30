#pragma once

#include <string>
#include "autobuf2.h"
#include "mac_address.h"
#include "ip_address.h"

/*
 * fast packets. compatible with std_packet, but evaluates fields lazily.
 * packets cannot be modified in-place.
 */

/*
 * abstract class for incoming packets
 */

using namespace std;
class raw_packet {

	// defines packet types
	enum class packet_t { RAW_PACKET, ARP_PACKET, ICMP_PACKET, IP_PACKET,
		TCP_PACKET, UDP_PACKET };

	// defines ethertypes
	enum class ethertype_t { IPV4, IPV6, ARP, RAID0, VLAN, LLDP };

	// constructor and destructor
	raw_packet() {}
	virtual ~raw_packet() {}

	// used to clear packet and to get debug information
	virtual void clear();
	virtual string to_string() const;

	// raw-packet specific information
	uint16_t    in_port() const;
	packet_t    packet_type() const;

	// reader methods
	mac_address src_mac() const;
	mac_address dst_mac() const;
	uint16_t    ethertype() const;
	ethertype_t ethertype_enum() const;
	autobuf     raw_payload() const;

	// writer methods
	void set_src_mac(const mac_address& src);
	void set_dst_mac(const mac_address& dst);
	void set_ethertype(uint16_t type);
	void set_ethertype(const ethertype_t& type);
	void set_raw_payload(const autobuf& buf);

	// used to serialize a raw packet (does not treat the packet as a higher
	// level protocol packet)
	uint32_t serialize(autobuf& dest) const;
	bool     deserialize(const autobuf& input);

protected:

	// used by higher level protocol packets to handle serialization
	uint32_t serialize_special(autobuf& dest) const;
	bool     deserialize_special(const autobuf& input);

	autobuf raw_packet_buf;
	uint16_t in_port;
	packet_t 
};


