#pragma once

#include "autobuf.h"
#include "common_utils.h"
#include "mac_address.h"
#include "ip_address.h"
#include "std_packet.h"

/**
 *  defines the ironscale packet used for communications
 *  with ironscale entities
**/
class ironscale_packet {
public:

	enum class ironscale_service_type { UNKNOWN = 0, PROBE = 1, HELLO = 2,
		HELLO_REPLY = 3, SERVICE_SETUP = 4};

	ironscale_packet():request_type(ironscale_service_type::UNKNOWN),
		request_id(0) {}

	// publicly accessible fields
	ironscale_service_type request_type;	// select from above enum class

	uint32_t request_id;	// used for matching request IDs

	mac_address src_mac;	// actual mac address of the origin entity sending
												// this ironscale packet
	mac_address dest_mac; // actual mac address destination of the recipient;
												// no need to fill in for probe messages
	autobuf payload;			// any other ancillary data to follow ironscale pkt


	// to check for equality
	bool operator==(const ironscale_packet& other) const;
	bool operator!=(const ironscale_packet& other) const;

	// clears the packet
	void clear();

	// used to quickly check if another udp packet is an ironscale packet
	static bool is_ironscale_packet(const udp_packet& pkt);

	// serialization functions
	uint32_t serialize(autobuf& buf);
	bool deserialize(const autobuf& buf);

	// enum conversion
	static ironscale_service_type uint16_to_service_type(uint16_t type);

private:

	typedef struct {
		uint8_t request_type[2];
		uint8_t request_id[4];
		uint8_t src_mac[6];			// real src mac, not the masked one
		uint8_t dest_mac[6];
		uint8_t payload[0];
	} ironscale_packet_hdr;
};
