#include "ironscale_packet.h"

// clears the packet
void ironscale_packet::clear() {
	request_type = ironscale_service_type::UNKNOWN;
	request_id = 0;
	src_mac.clear();
	dest_mac.clear();
	payload.clear();
}

// checks if a packet is an ironscale packet (not robust yet)
bool ironscale_packet::is_ironscale_packet(const udp_packet& pkt) {
	return (pkt.src_mac == mac_address("aa:aa:aa:aa:aa:aa") &&
		pkt.src_port == 10000 && pkt.dest_port == 10000 &&
		pkt.udp_payload.size() >= sizeof(ironscale_packet_hdr));
}

// checks for equality
bool ironscale_packet::operator==(const ironscale_packet& other) const {
	return request_type == other.request_type &&
		request_id == other.request_id &&
		src_mac == other.src_mac &&
		dest_mac == other.dest_mac &&
		payload == other.payload;
}

bool ironscale_packet::operator!=(const ironscale_packet& other) const {
	return !((*this) == other);
}

// serializes the ironscale packet
uint32_t ironscale_packet::serialize(autobuf& dest) {
	udp_packet pkt;
	pkt.src_mac = mac_address("aa:aa:aa:aa:aa:aa");
	pkt.dest_mac = dest_mac;
	pkt.src_ip = ip_address("0.0.0.0");
	pkt.dest_ip = ip_address("0.0.0.0");
	pkt.src_port = 10000;
	pkt.dest_port = 10000;
	
	ironscale_packet_hdr header;
	pack_uint16(header.request_type, (uint16_t) request_type);
	pack_uint32(header.request_id, request_id);
	src_mac.get(header.src_mac);
	dest_mac.get(header.dest_mac);
	pkt.udp_payload = autobuf(&header, sizeof(header)) + payload;

	return pkt.serialize(dest);
}

// deserializes an ironscale packet
bool ironscale_packet::deserialize(const autobuf& buf) {
	udp_packet pkt;
	
	if (!pkt.deserialize(buf) || pkt.src_mac != mac_address("aa:aa:aa:aa:aa:aa") ||
		pkt.src_port != 10000 || pkt.dest_port != 10000 ||
		pkt.udp_payload.size() < sizeof(ironscale_packet_hdr)) {
		return false;
	}

	ironscale_packet_hdr* hdr = (ironscale_packet_hdr*) pkt.udp_payload.get_content_ptr();
	
	request_type = uint16_to_service_type(unpack_uint16(hdr->request_type));
	request_id = unpack_uint32(hdr->request_id);
	src_mac.set_from_network_buffer(hdr->src_mac);
	dest_mac.set_from_network_buffer(hdr->dest_mac);

	uint32_t payload_size = pkt.udp_payload.size() - sizeof(ironscale_packet_hdr);
	if (payload_size > 0) {
		payload.set_content(hdr->payload, payload_size);
	} else {
		payload.clear();
	}

	return true;
}

ironscale_packet::ironscale_service_type ironscale_packet::uint16_to_service_type(uint16_t type) {
	if (type >= (uint16_t)ironscale_packet::ironscale_service_type::UNKNOWN &&
		type <= (uint16_t) ironscale_packet::ironscale_service_type::SERVICE_SETUP) {
		return (ironscale_packet::ironscale_service_type)type;
	}

	return ironscale_packet::ironscale_service_type::UNKNOWN;
}
