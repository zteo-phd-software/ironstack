#pragma once

#include <string>
#include "autobuf.h"
#include "mac_address.h"
#include "ip_address.h"
#include "ipv6_address.h"
#include "common_utils_oop.h"
using namespace std;

// abstract class for incoming packets
class raw_packet {
public:

	typedef enum { ARP_PACKET, ICMP_PACKET, TCP_PACKET, UDP_PACKET, DHCP_PACKET, UNKNOWN_PACKET, IPV6_PACKET, DROP_PACKET } packet_t;

	typedef struct {
		uint8_t ethernet_dest[6];
		uint8_t ethernet_src[6];
		uint8_t ethertype[2];
	} untagged_ethernet_hdr_t;

	typedef struct {
		uint8_t ethernet_dest[6];
		uint8_t ethernet_src[6];
		uint8_t vlan_8100[2];
		uint8_t vlan_pcp_dei_id[2];
		uint8_t ethertype[2];
	} tagged_ethernet_hdr_t;

	raw_packet();
	virtual ~raw_packet();

	virtual void clear();
	virtual std::string to_string() const;

	uint16_t get_vlan_id() const;

	// user accessible fields
	uint16_t in_port;						// input port (if generated by of_message_packet_in event; ignored by serializer and deserializer)
	packet_t packet_type;
	uint16_t ethertype;

	bool     has_vlan_tag;
	uint8_t  vlan_pcp;
	bool     dei;
	uint16_t vlan_id;

	mac_address src_mac;				// mac of the entity that forwarded the msg
	mac_address dest_mac;				// mac dest of the entity to relay/collect the msg

	// helper functions inherited and useful for all subclasses
	// similar to serialize and deserialize functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// arp packet class
class arp_packet : public raw_packet {
public:

	typedef struct {
		uint8_t hardware_type[2];
		uint8_t protocol_type[2];
		uint8_t hardware_address_len;
		uint8_t protocol_address_len;
		uint8_t operation[2];
		uint8_t sender_hardware_address[6];
		uint8_t sender_ip_address[4];
		uint8_t destination_hardware_address[6];
		uint8_t destination_ip_address[4];
	} arp_hdr_t;

	arp_packet();
	virtual ~arp_packet();

	virtual void clear();
	virtual std::string to_string() const;

	// user accessible data fields
	mac_address sender_mac;				// mac of the original sender
	mac_address receiver_mac;			// mac of the penultimate receiver

	bool arp_request;					// true means request, false means reply

	ip_address src_ip;
	ip_address dest_ip;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

class ipv6_packet : public raw_packet {
public:
  
  typedef struct {
    uint8_t version_tc_fl[4]; // 4 bits version, 8 bits traffic class, 20 bits flow label
    uint8_t payload_length[2];
    uint8_t next_header;
    uint8_t hop_limit;
    uint8_t src_ipv6[16];
    uint8_t dest_ipv6[16];
  } ipv6_hdr_t;
 
  typedef struct ipv6_ext_hdr {
    uint8_t next_header;
    uint8_t header_length;
  } ipv6_ext_hdr_t;

  ipv6_packet();
  virtual ~ipv6_packet();
  
  virtual void clear();
  virtual std::string to_string() const;

	/* ipv6 specific features */
  uint8_t version;
	uint8_t traffic_class; // 8 bits
	uint32_t flow_label; // 20 bits
	uint16_t payload_length; // 16 bits;
	uint8_t next_header; // 8 bits;
	uint8_t hop_limit; // *ttl* 8 bits;

  ipv6_address src_ipv6;
  ipv6_address dest_ipv6;

  autobuf ipv6_payload;

  virtual uint32_t serialize(autobuf& dest) const;
  virtual bool deserialize(const autobuf& input);

  uint32_t serialize_special(autobuf& dest, uint16_t payload_len) const;
};

// defines an IP packet -- but should not be instantiated on its own
class ip_packet : public raw_packet {
public:

	typedef struct {
		uint8_t version_ihl;			// upper 4 bits version, lower 4 bits IHL
		uint8_t dscp_ecn;					// upper 6 bits DSCP, lower 2 bits ECN
		uint8_t total_len[2];			// total len in bytes
		uint8_t identification[2];		// 
		uint8_t flags_frag_offset[2];	// upper 3 bits flags, lower 13 frag offset
		uint8_t ttl;
		uint8_t protocol; 
		uint8_t header_checksum[2];
		uint8_t src_ip[4];
		uint8_t dest_ip[4];
	} ip_hdr_t;

	ip_packet();
	virtual ~ip_packet();

	virtual void clear();
	virtual std::string to_string() const;


	/* ipv4 specific features */
	uint8_t version;		// 4 bits ipv4 vs ipv6
	uint8_t ihl;				// 4 bits
	uint8_t dscp;				// 6 bits
	uint8_t ecn;				// 2 bits

	uint16_t identification;	// 16 bits
	bool dont_fragment;				// combined with more fragments, 3 bits total
	bool more_fragments;

	uint16_t fragment_offset;	// 13 bits
	uint8_t ttl;							// 8 bits
	uint8_t protocol;					// 8 bits
	uint16_t ip_hdr_checksum;	// 16 bits

	ip_address src_ip;				// 32 bits
	ip_address dest_ip;				// 32 bits

	autobuf ip_options;				// only set if ihl dictates so
	autobuf ip_payload;				// set only for raw IP packets

	// serialization and deserialization methods
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

	// used in serialization for tcp/udp/IP dependent packets that
	// operate in multistage
	uint32_t serialize_special(autobuf& dest, uint16_t payload_len) const;

	static uint16_t checksum_ip(const ip_hdr_t* ip_hdr);
};

// defines a TCP packet
class tcp_packet : public ip_packet {
public:

	typedef struct {
		uint8_t src_port[2];
		uint8_t dest_port[2];
		uint8_t seq[4];
		uint8_t ack[4];
		uint8_t data_offset_reserved;
		uint8_t flags;
		uint8_t window_size[2];
		uint8_t checksum[2];
		uint8_t urgent_ptr[2];
		uint8_t data[0];
	} tcp_hdr_t;

	tcp_packet();
	virtual ~tcp_packet();

	virtual void clear();
	virtual std::string to_string() const;

	// user accessible data fields
	uint16_t src_port;
	uint16_t dest_port;

	uint32_t seq;
	uint32_t ack;

	bool ns_flag;
	bool cwr_flag;
	bool ece_flag;
	bool urg_flag;
	bool ack_flag;
	bool psh_flag;
	bool rst_flag;
	bool syn_flag;
	bool fin_flag;

	uint16_t window_size;
	uint16_t tcp_checksum;
	uint16_t urgent_ptr;

	autobuf options;
	autobuf payload;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

private:

	typedef struct {
		uint8_t src_ip[4];
		uint8_t dest_ip[4];
		uint8_t reserved;
		uint8_t protocol;
		uint8_t tcp_segment_len[2];
		uint8_t data[0];
		tcp_hdr_t tcp_hdr;
	} tcp_checksum_hdr_t;

	static uint16_t checksum_tcp(const ip_hdr_t* ip_hdr, const tcp_hdr_t* tcp_hdr, const autobuf& tcp_payload);
};

// defines a UDP packet
class udp_packet : public ip_packet {
public:

	typedef struct {
		uint8_t src_port[2];
		uint8_t dest_port[2];
		uint8_t len[2];
		uint8_t checksum[2];
		uint8_t data[0];
	} udp_hdr_t;

	udp_packet();
	virtual ~udp_packet();

	virtual void clear();
	virtual std::string to_string() const;

	// user accessible data fields
	uint16_t src_port;
	uint16_t dest_port;

	uint16_t udp_checksum;

	autobuf udp_payload;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

private:

	typedef struct {
		uint8_t src_ip[4];
		uint8_t dest_ip[4];
		uint8_t reserved;
		uint8_t protocol;
		uint8_t len[2];
		udp_hdr_t udp_hdr;
	} udp_checksum_hdr_t;

	static uint16_t checksum_udp(const ip_hdr_t* ip_hdr, const udp_hdr_t* udp_hdr, const autobuf& udp_payload);
};

// defines a DHCP packet
class dhcp_packet : public udp_packet {
public:

	typedef struct {
		uint8_t op;
		uint8_t htype;
		uint8_t hlen;
		uint8_t hops;

		uint8_t xid[4];
		uint8_t secs[2];
		uint8_t flags[2];

		uint8_t client_addr[4];
		uint8_t your_addr[4];
		uint8_t server_addr[4];
		uint8_t gateway_addr[4];

		uint8_t client_hwaddr[6];	// eth address
		uint8_t client_hwaddr_null[10]; // technically 16 bytes; this is ignored

		uint8_t bootp_fields[192];	// all 0s, should be ignored
		uint8_t magic_cookie[4];

	} dhcp_hdr_t;

	dhcp_packet();
	virtual ~dhcp_packet();

	bool request;
	uint8_t hardware_type;
	uint8_t hw_addr_len;
	uint8_t hops;

	uint32_t xid;
	uint16_t seconds;

	// flags
	bool is_broadcast;

	ip_address client_address;
	ip_address your_address;
	ip_address server_address;
	ip_address gateway_address;

	mac_address client_hw_address;
	autobuf dhcp_options; 

	virtual void clear();
	virtual std::string to_string() const;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};

// defines an ICMP packet
class icmp_packet : public ip_packet {
public:

	typedef struct {
		uint8_t type;
		uint8_t code;
		uint8_t checksum[2];
		uint8_t rest[4];
		uint8_t data[0];
	} icmp_hdr_t;

	icmp_packet();
	virtual ~icmp_packet();

	virtual void clear();
	virtual std::string to_string() const;

	// user accessible data fields
	uint8_t icmp_pkt_type;
	uint8_t code;
	uint16_t checksum;
	uint32_t icmp_hdr_rest;

	autobuf data;

	// inherited from serializable class
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

private:

	static uint16_t checksum_icmp(const icmp_hdr_t* icmp_hdr, uint32_t size);
//	static uint16_t checksum_icmp(const icmp_hdr_t* icmp_hdr, const autobuf& data);

};

// utilities to convert between vlan packets and non vlan packets
namespace std_packet_utils {

	bool set_vlan_tag(const autobuf& input, autobuf& output, uint16_t vlan);
	bool strip_vlan_tag(const autobuf& input, autobuf& output);

}

// factory class that generates the correct packet type
class std_packet_factory {
public:
	static raw_packet* instantiate(const autobuf& data);
	static raw_packet* instantiate(autobuf* msg_buf, const autobuf& data);
};
