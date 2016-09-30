#include "autobuf.h"
#include "mac_address.h"
#include "ip_address.h"

/* 
 * this is the updated class for standard packets. all fields are eager
 * evaluated, which could mean that instantiating packets this way is
 * slower. for the faster packets, use std_fast_packet.h.
 */

// raw ethernet packets
class raw_packet {
public:

	typedef enum { ARP_PACKET, ICMP_PACKET, TCP_PACKET, UDP_PACKET, DHCP_PACKET,
		UNKNOWN_PACKET, IPV4_PACKET, IPV6_PACKET, DROP_PACKET } packet_t;

	// raw ethernet header
	typedef struct {
		uint8_t ethernet_dest[6];
		uint8_t ethernet_src[6];
		uint8_t ethertype[2];
	} ethernet_hdr_t;

	raw_packet();
	virtual ~raw_packet();

	virtual void clear();
	virtual std::string to_string() const;

	// user accessible-fields
	uint16_t in_port;
	packet_t packet_type;
	uint16_t ethertype;

	mac_address src_mac;
	mac_address dest_mac;

	autobuf raw_packet_payload;

	// helper functions

	// use this if you want to generate an ethernet packet with possibly custom
	// headers on top of the ethernet header
	virtual uint32_t serialize(autobuf& dest) const;

	// this is used by higher-level packets dependent on ethernet packets. dest
	// must be preallocated. bounds check will not be performed.
	uint32_t serialize_for_higher_level(const autobuf& dest) const;

	// deserializes the raw packet
	virtual bool deserialize(const autobuf& input);
};


