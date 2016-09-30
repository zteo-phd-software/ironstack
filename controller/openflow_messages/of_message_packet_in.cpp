#include "of_message_packet_in.h"
using namespace std;

bool of_message_packet_in::display_payload = false;

// constructor
of_message_packet_in::of_message_packet_in() {
	clear();
}

// clears the object
void of_message_packet_in::clear() {
	buffer_id = 0;
	in_port = 0;
	reason_no_match = false;
	reason_action = false;
	pkt_data.clear();
}

// generates a user-readable debug string
string of_message_packet_in::to_string() const {
	char buf[128];
	string special_port;
	string result = of_message::to_string();
	result += "\npacket in attributes:\n";
	sprintf(buf, "buffer id: %u\n", buffer_id);
	result += buf;
	special_port = of_types::port_to_string(in_port);
	if (special_port.size() == 0) {
		sprintf(buf, "in port  : %d\n", in_port);
	} else {
		sprintf(buf, "in port  : %s\n", special_port.c_str());
	}
	result += buf;
	result += "reason: ";
	if (reason_no_match) {
		result += "no matching flow";
	}	else if (reason_action) {
		result += "action requested by controller";
	}	else {
		result += "ERROR";
	}
	result += "\n";
	sprintf(buf, "packet size : %u bytes", pkt_data.size());
	result += buf;

	if (summarized) {
		result += "\npacket contents have been summarized.";
	}	else {
		result += "\nfull-length packet contents are as follows.";
	}

	if (!display_payload) {
		result += "\npacket contents display suppressed.";
	}	else 	{
		result += "\n" + of_common::binary_to_hex_output(pkt_data);
	}
	
	return result;
}

// should never be called -- controller never sends this message type
uint32_t of_message_packet_in::serialize(autobuf& dest) const {
	abort();
	return 0;
}

// deserializes the raw data into this message type
bool of_message_packet_in::deserialize(const autobuf& input) {
	clear();
	bool status = of_message::deserialize(input);
	struct ofp_packet_in* hdr = (struct ofp_packet_in*) input.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (!status) {
		goto fail;
	}

	// sanity check the message type
	if (msg_type != OFPT_PACKET_IN) {
		goto fail;
	}
	#endif

	buffer_id = ntohl(hdr->buffer_id);
	actual_message_len = ntohs(hdr->total_len);
	in_port = ntohs(hdr->in_port);
	
	if (hdr->reason == (uint8_t) OFPR_NO_MATCH) {
		reason_no_match = true;
	}	else if (hdr->reason == (uint8_t) OFPR_ACTION) {
		reason_action = true;
	}

	pkt_data.set_content((void*)input.ptr_offset_const(sizeof(struct ofp_packet_in)-2), actual_message_len);

	if (pkt_data.size() != actual_message_len) {
		summarized = true;
	}	else {
		summarized = false;
	}

	return true;

fail:
	clear();
	return false;
}

// turns on or off message packet display
void of_message_packet_in::show_contents(bool state) {
	display_payload = state;
}

string of_message_packet_in::get_breakdowns() const {
	char buf[512];
	string result;
	mac_address src, dest;

	// structure for ethernet packet
	struct {
		uint8_t ethernet_dest[6];
		uint8_t ethernet_src[6];
		uint8_t ethertype[2];
	} ethernet_hdr;

	// sanity check
	if (pkt_data.size() < sizeof(ethernet_hdr)) {
		return "packet error -- ethernet header is invalid.\n";
	}

	// copy in and parse fields
	memcpy(&ethernet_hdr, pkt_data.get_content_ptr(), sizeof(ethernet_hdr));
	dest.set_from_network_buffer(ethernet_hdr.ethernet_dest);
	src.set_from_network_buffer(ethernet_hdr.ethernet_src);
	sprintf(buf, "ethernet dest: [%s]\n", dest.to_string().c_str());
	result += buf;
	sprintf(buf, "ethernet src : [%s]\n", src.to_string().c_str());
	result += buf;

	// ethertype 0x0806 is ARP request
	switch (unpack_uint16(ethernet_hdr.ethertype)) {
		// ARP request
		case 0x0806:
			
			struct {
				uint8_t hardware_type[2];
				uint8_t protocol_type[2];
				uint8_t hardware_address_len;
				uint8_t protocol_address_len;
				uint8_t operation[2];
				uint8_t sender_hardware_address[6];
				uint8_t sender_ip_address[4];
				uint8_t destination_hardware_address[6];
				uint8_t destination_ip_address[4];
			} arp_hdr;

			if (pkt_data.size() != sizeof(ethernet_hdr) + sizeof(arp_hdr)) {
				return "packet error -- arp packet is invalid.\n";
			}
			memcpy(&arp_hdr, ((uint8_t*)pkt_data.get_content_ptr())+sizeof(ethernet_hdr), sizeof(arp_hdr));

			result += "ARP ";
			if (unpack_uint16(arp_hdr.operation) == 1) {
				result += "request message.\n";
			} else if (unpack_uint16(arp_hdr.operation) == 2) {
				result += "reply message.\n";
			}	else {
				result += "unknown message type.\n";
			}
			
			uint16_t hardware_type = unpack_uint16(arp_hdr.hardware_type);
			uint16_t protocol_type = unpack_uint16(arp_hdr.protocol_type);

			sprintf(buf, "hardware type: [%d]", hardware_type);
			result += buf;
			if (hardware_type == 1) {
				result += " ethernet\n";
			}	else {
				result += "\n";
			}
			
			sprintf(buf, "protocol type: [0x%04x]", protocol_type);
			result += buf;
			if (protocol_type == 0x0800) {
				result += " IPv4\n";
			}	else {
				result += "\n";
			}

			sprintf(buf, "hw addr len: [%u]", arp_hdr.hardware_address_len);
			result += buf;
			if (arp_hdr.hardware_address_len == 6) {
				result += " ethernet\n";
			}	else {
				result += "\n";
			}

			sprintf(buf, "proto addr len: [%u]", arp_hdr.protocol_address_len);
			result += buf;
			if (arp_hdr.protocol_address_len == 4) {
				result += " IPv4\n";
			}	else {
				result += "\n";
			}

			mac_address sender_hw_addr(arp_hdr.sender_hardware_address);
			mac_address destination_hw_addr(arp_hdr.destination_hardware_address);
			ip_address sender_ip_addr;
			sender_ip_addr.set_from_network_buffer(&arp_hdr.sender_ip_address);
			ip_address destination_ip_addr;
			destination_ip_addr.set_from_network_buffer(&arp_hdr.destination_ip_address);

			result += "sender mac [" + sender_hw_addr.to_string();
			result += "]\nsender IP  [" + sender_ip_addr.to_string();
			result += "]\ndestination mac [" + destination_hw_addr.to_string();
			result += "]\ndestination IP  [" + destination_ip_addr.to_string();
			result += "]\n";

			break;
	}

	return result;
}

// instantiates the correct version of the actual underlying packet, given
// the message_in packet
raw_packet* packet_factory::instantiate(autobuf* buf, const of_message_packet_in& msg) {

	raw_packet* result = std_packet_factory::instantiate(buf, msg.pkt_data);
	if (result != nullptr) {
		result->in_port = msg.in_port;
	}

	return result;
}

