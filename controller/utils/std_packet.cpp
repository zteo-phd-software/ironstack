#include "std_packet.h"
#include "../gui/output.h"

// base class constructor
raw_packet::raw_packet() {
	in_port = 0xffff; // (uint16_t) OFPP_NONE;
	ethertype = 0;
	has_vlan_tag = false;
	vlan_pcp = 0;
	dei = false;
	vlan_id = 1;
	packet_type = UNKNOWN_PACKET;
}

// destructor
raw_packet::~raw_packet() {
}

// clears common fields
void raw_packet::clear() {
	in_port = 0xffff; // (uint16_t) OFPP_NONE;
	packet_type = UNKNOWN_PACKET;
	ethertype = 0;
	has_vlan_tag = false;
	vlan_pcp = 0;
	dei = false;
	vlan_id = 1;
	src_mac.clear();
	dest_mac.clear();
}

// generates a readable form of the packet
string raw_packet::to_string() const {
	string result;
	char buf[16];

	switch (packet_type) {
		case ARP_PACKET:
			result += "packet type     : ARP";
			break;
		case TCP_PACKET:
			result += "packet type     : TCP";
			break;
		case UDP_PACKET:
			result += "packet type     : UDP";
			break;
		case DHCP_PACKET:
			result += "packet type     : DHCP";
			break;
		default:
			result += "packet type     : UNKNOWN";
	}
	sprintf(buf, "%hu", ethertype);
	result += "\nethertype       : " + string(buf);
	result += "\nvlan tagged     : " + has_vlan_tag ? string("yes") : string("no");
	if (has_vlan_tag) {
		result += "\nvlan id         : " + string(buf);
		sprintf(buf, "%hu", vlan_pcp);
		result += "\nvlan pcp        : " + string(buf);
		result += "\nvlan drop elig. : " + dei ? string("yes") : string("no");
		sprintf(buf, "%hu", vlan_id);
	}

	result += "\nsrc mac         : " + src_mac.to_string();
	result += "\ndest mac        : " + dest_mac.to_string();

	return result;
}

// returns the vlan id of this packet, or 1 if not specified
uint16_t raw_packet::get_vlan_id() const {
	if (has_vlan_tag) {
		return vlan_id;
	} else {
		return 1;
	}
}

// generates the common ethernet header for all dependent packets
uint32_t raw_packet::serialize(autobuf& dest) const {

	// process vlan tagged packets separately from untagged packets
	if (has_vlan_tag) {
		dest.create_empty_buffer(sizeof(tagged_ethernet_hdr_t), false);
		tagged_ethernet_hdr_t* eth_hdr = (tagged_ethernet_hdr_t*)dest.get_content_ptr_mutable();

		dest_mac.get(eth_hdr->ethernet_dest);
		src_mac.get(eth_hdr->ethernet_src);

		pack_uint16(eth_hdr->vlan_8100, 0x8100);
		uint16_t pcp_dei_id = ((vlan_pcp & 0x07) << 13) | (dei ? (1 << 12) : 0) | (vlan_id & 0x0fff);

		pack_uint16(eth_hdr->vlan_pcp_dei_id, pcp_dei_id);
	} else {
		dest.create_empty_buffer(sizeof(untagged_ethernet_hdr_t), false);
		untagged_ethernet_hdr_t* eth_hdr = (untagged_ethernet_hdr_t*)dest.get_content_ptr_mutable();

		dest_mac.get(eth_hdr->ethernet_dest);
		src_mac.get(eth_hdr->ethernet_src);
	}

	// figure out the ethertype
	uint16_t ethertype_to_pack = ethertype;
	if (ethertype == 0) {
		switch (packet_type) {
			case ARP_PACKET:
				ethertype_to_pack = 0x0806;
				break;
			case TCP_PACKET:
			case UDP_PACKET:
			case DHCP_PACKET:
			case ICMP_PACKET:
				ethertype_to_pack = 0x0800;
				break;
			case IPV6_PACKET:
				ethertype_to_pack = 0x86dd;
				break;
			default:
				// error
				output::log(output::loglevel::BUG, "raw_packet::serialize() -- cannot serialize a packet with unknown packet type.\n");
				assert(false && "raw_packet::serialize() -- cannot serialize a packet with unknown packet type.");
				abort();
		}
	}

	// pack the ethertype
	if (has_vlan_tag) {
		tagged_ethernet_hdr_t* eth_hdr = (tagged_ethernet_hdr_t*)dest.get_content_ptr_mutable();
		pack_uint16(eth_hdr->ethertype, ethertype_to_pack);
		return sizeof(tagged_ethernet_hdr_t);

	} else {
		untagged_ethernet_hdr_t* eth_hdr = (untagged_ethernet_hdr_t*)dest.get_content_ptr_mutable();
		pack_uint16(eth_hdr->ethertype, ethertype_to_pack);
		return sizeof(untagged_ethernet_hdr_t);
	}
}

// deserializes the common ethernet header and updates the packet type
bool raw_packet::deserialize(const autobuf& pkt_data) {

	const untagged_ethernet_hdr_t* untagged_eth_hdr = (const untagged_ethernet_hdr_t*) pkt_data.get_content_ptr();

	// sanity check
	if (pkt_data.size() < sizeof(untagged_ethernet_hdr_t)) {
		output::log(output::loglevel::BUG, "raw_packet::deserialize() -- bad ethernet header.\n"
			"expected header size: %u bytes, actual header: %u bytes.\n"
			"contents of packet as follows:\n%s\n",
			(uint32_t) sizeof(untagged_ethernet_hdr_t),
			pkt_data.size(),
			pkt_data.to_hex().c_str());
		return false;
	}

	// parse fields
	dest_mac.set_from_network_buffer(untagged_eth_hdr->ethernet_dest);
	src_mac.set_from_network_buffer(untagged_eth_hdr->ethernet_src);

	// check for supported ethertypes
	ethertype = unpack_uint16(untagged_eth_hdr->ethertype);

	// if this is a vlan, handle it separately
	uint32_t actual_hdr_len = 0;
	if (ethertype == 0x8100) {
		if (pkt_data.size() < sizeof(tagged_ethernet_hdr_t)) {
			output::log(output::loglevel::BUG, "raw_packet::deserialize() -- bad ethernet tagged header.\n"
				"expected header size: %u bytes, actual header: %u bytes.\n"
				"contents of packet as follows:\n%s\n",
				(uint32_t) sizeof(tagged_ethernet_hdr_t),
				pkt_data.size(),
				pkt_data.to_hex().c_str());
			return false;
		}

		const tagged_ethernet_hdr_t* tagged_eth_hdr = (const tagged_ethernet_hdr_t*) pkt_data.get_content_ptr();
		actual_hdr_len = sizeof(tagged_ethernet_hdr_t);
		has_vlan_tag = true;
		uint16_t vlan_pcp_dei_id = unpack_uint16(tagged_eth_hdr->vlan_pcp_dei_id);
		vlan_pcp = vlan_pcp_dei_id >> 13;
		dei = ((vlan_pcp_dei_id >> 12) & 0x01) > 0;
		vlan_id = vlan_pcp_dei_id & 0x0fff;
		ethertype = unpack_uint16(tagged_eth_hdr->ethertype);
	} else {
		actual_hdr_len = sizeof(untagged_ethernet_hdr_t);
		has_vlan_tag = false;
		vlan_pcp = 0;
		dei = false;
		vlan_id = 1;
	}

	// setup the raw packet type
	switch (ethertype) {
		case 0x0806:
		{
			// ARP
			packet_type = ARP_PACKET;
			break;
		}

		case 0x0800:
		{
			// some kind of IPv4 -- find its subtype from the IP header
			if (pkt_data.size() < actual_hdr_len + 20) {
				output::log(output::loglevel::BUG, "raw_packet::deserialize() -- bad IP header.\n"
					"expected size: %u bytes, actual size: %u bytes.\n",
					"contents of packet as follows:\n%s\n",
					actual_hdr_len+20,
					pkt_data.size(),
					pkt_data.to_hex().c_str());
				packet_type = UNKNOWN_PACKET;
			} else {
				switch (pkt_data[actual_hdr_len+9]) {
					case 0x01:
						packet_type = ICMP_PACKET;
						break;
					case 0x06:
						packet_type = TCP_PACKET;
						break;
					case 0x11:
						packet_type = UDP_PACKET;
						break;

					// TODO -- dhcp?

					default:
						output::log(output::loglevel::WARNING, "raw_packet::deserialize() -- unsupported IPv4 packet type [%hu]\n"
							"contents of packet as follows:\n%s\n",
							pkt_data[actual_hdr_len+9],
							pkt_data.to_hex().c_str());
						packet_type = UNKNOWN_PACKET;
				};
			}
			break;
		}
		
		case 0x86dd:
		{
			// IPv6 -- ignore for now
			packet_type = IPV6_PACKET;
			break;
		}

		default:
		{
			output::log(output::loglevel::WARNING, "raw_packet::deserialize() -- unsupported ethernet packet type [0x%04x]\n"
				"contents of packet as follows:\n%s\n",
				ethertype,
				pkt_data.to_hex().c_str());
			packet_type = UNKNOWN_PACKET;
		}
	}

	return packet_type != UNKNOWN_PACKET;
}

// arp packet
arp_packet::arp_packet() {
	packet_type = ARP_PACKET;
}

// virtual destructor
arp_packet::~arp_packet() {
}

// clears the arp packet
void arp_packet::clear() {
	raw_packet::clear();
	packet_type = ARP_PACKET;

	sender_mac.clear();
	receiver_mac.clear();
	arp_request = true;
	src_ip.clear();
	dest_ip.clear();
}

// generates a readable version of the arp packet
string arp_packet::to_string() const {
	string result = raw_packet::to_string();
	result += arp_request ? "\nARP request" : "\nARP reply";
	result += "\nsender MAC address: " + sender_mac.to_string();
	result += "\nreceiver MAC address: " + receiver_mac.to_string();
	result += "\nsrc IP address: " + src_ip.to_string();
	result += "\ndest IP address: " + dest_ip.to_string();

	return result;
}

// serializes the ARP packet for transmission
uint32_t arp_packet::serialize(autobuf& dest) const {

	uint32_t base_size = raw_packet::serialize(dest);
	uint32_t full_size = base_size+sizeof(arp_hdr_t);
	dest.create_empty_buffer(full_size, false);
	arp_hdr_t* arp_hdr = (arp_hdr_t*)dest.ptr_offset_mutable(base_size);

	pack_uint16(arp_hdr->hardware_type, 1);
	pack_uint16(arp_hdr->protocol_type, 0x0800);
	arp_hdr->hardware_address_len = 6;
	arp_hdr->protocol_address_len = 4;
	pack_uint16(arp_hdr->operation, arp_request ? 1 : 2);		 // 1 is request, 2 is reply
	sender_mac.get(arp_hdr->sender_hardware_address);
	
	src_ip.get_address_network_order(&arp_hdr->sender_ip_address);
	receiver_mac.get(arp_hdr->destination_hardware_address);
	dest_ip.get_address_network_order(&arp_hdr->destination_ip_address);

	return full_size;
}

// deserializes the ARP packet
bool arp_packet::deserialize(const autobuf& pkt_data) {

	// deserialize the ethernet header
	if (!raw_packet::deserialize(pkt_data)) {
		return false;
	}

	uint32_t eth_hdr_len = (has_vlan_tag ? sizeof(tagged_ethernet_hdr_t) : sizeof(untagged_ethernet_hdr_t));
	arp_hdr_t* arp_hdr = (arp_hdr_t*) pkt_data.ptr_offset_const(eth_hdr_len);

	#ifndef __NO_PACKET_SAFETY_CHECKS
	if (pkt_data.size() < eth_hdr_len + sizeof(arp_hdr_t)) {
		output::log(output::loglevel::BUG, "arp_packet::deserialize() -- header size incorrect.\n"
			"expected: %u bytes, actual: %u bytes.\n"
			"contents of packets as follows:\n%s\n",
			eth_hdr_len+sizeof(arp_hdr_t),
			pkt_data.size(),
			pkt_data.to_hex().c_str());
		return false;
	}
	#endif

	uint16_t hardware_type = unpack_uint16(arp_hdr->hardware_type);
	uint16_t protocol_type = unpack_uint16(arp_hdr->protocol_type);
	uint16_t op_type = unpack_uint16(arp_hdr->operation);

	// hardware_type == 1 means ethernet, protocol_type == 0x0800 means IPv4
	if (hardware_type != 1 || protocol_type != 0x0800 || arp_hdr->hardware_address_len != 6 || arp_hdr->protocol_address_len != 4 || op_type < 1 || op_type > 2) {
		output::log(output::loglevel::BUG, "arp_packet::deserialize() -- composite checks fail.\n"
			"contents of packets as follows:\n%s\n", pkt_data.to_hex().c_str());
		return false;
	}

	arp_request = (op_type == 1);
	sender_mac.set_from_network_buffer(arp_hdr->sender_hardware_address);
	src_ip.set_from_network_buffer(&arp_hdr->sender_ip_address);

	receiver_mac.set_from_network_buffer(arp_hdr->destination_hardware_address);
	dest_ip.set_from_network_buffer(&arp_hdr->destination_ip_address);

	return true;
}

// ipv6 packet

ipv6_packet::ipv6_packet() {
	traffic_class = 0;
	flow_label = 0;
	payload_length = 0;
	next_header = 59; // means there's no header afterwards. paylod length = 0
	hop_limit = 64;
}

ipv6_packet::~ipv6_packet() {
}

void ipv6_packet::clear() {
	raw_packet::clear();
	
	traffic_class = 0;
	flow_label = 0;
	payload_length = 0;
	next_header = 59;
	hop_limit = 64;

	src_ipv6.clear();
	dest_ipv6.clear();

	ipv6_payload.clear();
}

// generates a readable form of the IP packet
string ipv6_packet::to_string() const {
	char buf[16];
	string result = raw_packet::to_string();
	sprintf(buf, "%d", hop_limit);
	result += string("\nsrc  IP: ") + src_ipv6.to_string();
	result += string("\ndest IP: ") + dest_ipv6.to_string();
	result += "\nhop_limit: ";
	result += buf;

	return result;
}

bool ipv6_packet::deserialize(const autobuf& input) {
  if (!raw_packet::deserialize(input)) {
		output::log(output::loglevel::BUG, "ipv6_packet::deserialize() -- raw packet deserialization failed.\n");
    return false;
  }

	uint32_t eth_hdr_len = (has_vlan_tag ? sizeof(tagged_ethernet_hdr_t) : sizeof(untagged_ethernet_hdr_t));
  const ipv6_hdr_t* ipv6_hdr = (const ipv6_hdr_t*)input.ptr_offset_const(eth_hdr_len);
  uint32_t version_tc_fl = unpack_uint32(ipv6_hdr->version_tc_fl);

  version >>= 28;
  if (version != 6) {
    output::log(output::loglevel::BUG, "ipv6_packet::deserialize() -- incorrect version. expected 6, actual: %u\n", version);
    return false;
  }
  traffic_class = (version_tc_fl >> 20) << 4;
  flow_label = version_tc_fl << 12;
  payload_length = unpack_uint16(ipv6_hdr->payload_length);
  next_header = ipv6_hdr->next_header;
  hop_limit = ipv6_hdr->hop_limit;
  src_ipv6.set_from_network_buffer(&ipv6_hdr->src_ipv6);
  dest_ipv6.set_from_network_buffer(&ipv6_hdr->dest_ipv6);

  ipv6_payload.inherit_read_only(input.ptr_offset_const(eth_hdr_len + sizeof(ipv6_hdr_t)), payload_length); 

  //TODO: double check this;
  uint32_t total_extension_hdr_len = 0;
  const ipv6_ext_hdr_t* ipv6_ext_hdr = (const ipv6_ext_hdr_t*)input.ptr_offset_const(eth_hdr_len + sizeof(ipv6_hdr_t));
  while ( next_header == 0 || next_header == 60 || next_header == 43 || next_header == 44 || next_header == 51 || next_header == 50 || next_header == 135) {
    if (next_header == 50) {
      output::log(output::loglevel::BUG, "ipv6_packet::deserialize() -- this extension hdr not supported.");
			return false;
    } else if (next_header == 44) {
      total_extension_hdr_len += 8; // 64 bits, 8 bytes
    } else {
      total_extension_hdr_len += ipv6_ext_hdr->header_length + 1;
    }
    next_header = ipv6_ext_hdr->next_header;
    ipv6_ext_hdr = (const ipv6_ext_hdr_t*)input.ptr_offset_const(eth_hdr_len + sizeof(ipv6_hdr_t) + total_extension_hdr_len);
  }

  // resize the ipv6 payload to exclude extension headers
	if (payload_length - total_extension_hdr_len > 0) {
		ipv6_payload.inherit_read_only(input.ptr_offset_const(eth_hdr_len + sizeof(ipv6_hdr_t) + total_extension_hdr_len), payload_length - total_extension_hdr_len);
	}

	return true;
}

uint32_t ipv6_packet::serialize(autobuf& dest) const {
	uint32_t base_size = raw_packet::serialize(dest);
	uint32_t full_size = base_size + sizeof(ipv6_hdr_t) + ipv6_payload.size();
	dest.create_empty_buffer(full_size, false);

	ipv6_hdr_t* ipv6_hdr = (ipv6_hdr_t*) dest.ptr_offset_mutable(base_size);
	pack_uint32(ipv6_hdr->version_tc_fl, *((uint32_t*)ipv6_hdr->version_tc_fl) | flow_label | (version << 28) | (traffic_class << 20));
	pack_uint16(ipv6_hdr->payload_length, sizeof(ipv6_hdr_t) + ipv6_payload.size());
	ipv6_hdr->next_header = next_header;
	ipv6_hdr->hop_limit = hop_limit;
	src_ipv6.get_address_network_order(&ipv6_hdr->src_ipv6);
	dest_ipv6.get_address_network_order(&ipv6_hdr->dest_ipv6);

	if (ipv6_payload.size() > 0) {
		memcpy(dest.ptr_offset_mutable(base_size+sizeof(ipv6_hdr_t)), ipv6_payload.get_content_ptr(), ipv6_payload.size());
	}
	return full_size;
}

uint32_t ipv6_packet::serialize_special(autobuf& dest, uint16_t payload_len) const {
	uint32_t base_size = raw_packet::serialize(dest); 
	uint32_t full_size = base_size + sizeof(ipv6_hdr_t) + payload_len;
	dest.create_empty_buffer(full_size, false);

	ipv6_hdr_t* ipv6_hdr = (ipv6_hdr_t*) dest.ptr_offset_mutable(base_size);
	pack_uint32(ipv6_hdr->version_tc_fl, *((uint32_t*)ipv6_hdr->version_tc_fl) | flow_label | (version << 28) | (traffic_class << 20));
	pack_uint16(ipv6_hdr->payload_length, sizeof(ipv6_hdr_t) + ipv6_payload.size());
	ipv6_hdr->next_header = next_header;
	ipv6_hdr->hop_limit = hop_limit;
	src_ipv6.get_address_network_order(&ipv6_hdr->src_ipv6);
	dest_ipv6.get_address_network_order(&ipv6_hdr->dest_ipv6);

	return full_size;
}

// ip packet
ip_packet::ip_packet() {
	version = 4;
	ihl = 5;
	dscp = 0;
	ecn = 0;
	identification = 54635;
	dont_fragment = true;
	more_fragments = false;
	fragment_offset = 0;
	ttl = 64;
	protocol = 0;
	ip_hdr_checksum = 0;
}


// virtual destructor
ip_packet::~ip_packet() {
}

// clears the ip packet
void ip_packet::clear() {
	raw_packet::clear();

	version = 4;
	ihl = 5;
	dscp = 0;
	ecn = 0;

	identification = 54635;
	dont_fragment = true;
	more_fragments = false;

	fragment_offset = 0;
	ttl = 64;
	protocol = 0;
	ip_hdr_checksum = 0;

	src_ip.clear();
	dest_ip.clear();

	ip_options.clear();
	ip_payload.clear();
}

// generates a readable form of the IP packet
string ip_packet::to_string() const {
	char buf[16];
	string result = raw_packet::to_string();
	result += "\nsrc IP          : " + src_ip.to_string();
	result += "\ndest IP         : " + dest_ip.to_string();

	sprintf(buf, "%u", ttl);
	result += "\nttl             : ";
	result += buf;

	sprintf(buf, "%hu", ip_hdr_checksum);
	result += "\nip checksum     : ";
	result += buf;

	sprintf(buf, "%u", version);
	result += "\nversion         : ";
	result += buf;

	sprintf(buf, "%u", ihl);
	result += "\nihl             : ";
	result += buf;

	sprintf(buf, "%u", dscp);
	result += "\ndscp            : ";
	result += buf;

	sprintf(buf, "%u", ecn);
	result += "\necn             : ";
	result += buf;

	sprintf(buf, "%u", identification);
	result += "\nidentification  : ";
	result += buf;

	sprintf(buf, "%u", fragment_offset);
	result += "\nfragment offset : ";
	result += buf;

	result += "\nflags           : ";
	if (dont_fragment) {
		result += "DONT_FRAGMENT ";
	}
	if (more_fragments) {
		result += "MORE_FRAGMENTS";
	}

	return result;
}

// generates the packet up to the ip headers
// note: only use this for raw IP packets. UDP/TCP/IP-dependent packets should call
// the lower stage serialize_special() function
uint32_t ip_packet::serialize(autobuf& dest) const {
	uint32_t base_size = raw_packet::serialize(dest);
	uint32_t full_size = base_size + sizeof(ip_hdr_t) + ip_options.size() + ip_payload.size();
	dest.create_empty_buffer(full_size, false);

	ip_hdr_t* ip_hdr = (ip_hdr_t*) dest.ptr_offset_mutable(base_size);
	uint16_t checksum = 0;

	ip_hdr->version_ihl = (version << 4) | (ihl & 0x0f);
	ip_hdr->dscp_ecn = (dscp << 6) | (ecn & 0x03);
	pack_uint16(ip_hdr->total_len, sizeof(ip_hdr_t)+ip_options.size()+ip_payload.size());
	pack_uint16(ip_hdr->identification, identification);
	ip_hdr->flags_frag_offset[0] = (fragment_offset >> 8) & 0x1f;
	ip_hdr->flags_frag_offset[0] |= (dont_fragment ? (1 << 6) : 0);
	ip_hdr->flags_frag_offset[0] |= (more_fragments ? (1 << 5) : 0);
	ip_hdr->flags_frag_offset[1] = fragment_offset & 0xff;
	ip_hdr->ttl = ttl;
	ip_hdr->protocol = protocol;
	ip_hdr->header_checksum[0] = 0;
	ip_hdr->header_checksum[1] = 0;

	src_ip.get_address_network_order(&ip_hdr->src_ip);
	dest_ip.get_address_network_order(&ip_hdr->dest_ip);

	checksum = checksum_ip(ip_hdr);
	pack_uint16(ip_hdr->header_checksum, checksum);

	if (ip_options.size() > 0) {
		memcpy(dest.ptr_offset_mutable(base_size+sizeof(ip_hdr_t)), ip_options.get_content_ptr(), ip_options.size());
	}

	if (ip_payload.size() > 0) {
		memcpy(dest.ptr_offset_mutable(base_size+sizeof(ip_hdr_t)+ip_options.size()), ip_payload.get_content_ptr(), ip_payload.size());
	}

	return full_size;
}

// lower stage serialization for IP-dependent packets
uint32_t ip_packet::serialize_special(autobuf& dest, uint16_t payload_len) const {
	uint32_t base_size = raw_packet::serialize(dest);
	uint32_t full_size = base_size + sizeof(ip_hdr_t) + ip_options.size() + payload_len;
	dest.create_empty_buffer(full_size, false);

	ip_hdr_t* ip_hdr = (ip_hdr_t*) dest.ptr_offset_mutable(base_size);
	uint16_t checksum = 0;

	ip_hdr->version_ihl = (version << 4) | (ihl & 0x0f);
	ip_hdr->dscp_ecn = (dscp << 6) | (ecn & 0x03);
	pack_uint16(ip_hdr->total_len, sizeof(ip_hdr_t)+ip_options.size()+payload_len);
	pack_uint16(ip_hdr->identification, identification);
	ip_hdr->flags_frag_offset[0] = (fragment_offset >> 8) & 0x1f;
	ip_hdr->flags_frag_offset[0] |= (dont_fragment ? (1 << 6) : 0);
	ip_hdr->flags_frag_offset[0] |= (more_fragments ? (1 << 5) : 0);
	ip_hdr->flags_frag_offset[1] = fragment_offset & 0xff;
	ip_hdr->ttl = ttl;
	ip_hdr->protocol = protocol;
	ip_hdr->header_checksum[0] = 0;
	ip_hdr->header_checksum[1] = 0;

	src_ip.get_address_network_order(&ip_hdr->src_ip);
	dest_ip.get_address_network_order(&ip_hdr->dest_ip);

	checksum = checksum_ip(ip_hdr);
	pack_uint16(ip_hdr->header_checksum, checksum);

	if (ip_options.size() > 0) {
		memcpy(dest.ptr_offset_mutable(base_size+sizeof(ip_hdr_t)), ip_options.get_content_ptr(), ip_options.size());
	}

	return full_size;
}

// deserializes the ip hdr
bool ip_packet::deserialize(const autobuf& input) {
	uint16_t total_len = 0;
	uint16_t identification = 0;

	if (!raw_packet::deserialize(input)) {
		output::log(output::loglevel::BUG, "ip_packet::deserialize() -- raw packet deserialization failed.\n");
		return false;
	}

	uint32_t ethernet_hdr_len = (has_vlan_tag ? sizeof(tagged_ethernet_hdr_t) : sizeof(untagged_ethernet_hdr_t));
	const ip_hdr_t* ip_hdr = (const ip_hdr_t*)input.ptr_offset_const(ethernet_hdr_len);

	version = ip_hdr->version_ihl >> 4;
	if (version != 4) {
		// version not supported
		output::log(output::loglevel::BUG, "ip_packet::deserialize() -- incorrect version. expected 4, actual: %u.\n", version);
		return false;
	}

	ihl = ip_hdr->version_ihl & 0x0f;
	if (ihl != 5) {
		// options not supported
		output::log(output::loglevel::BUG, "ip_packet::deserialize() -- options not supported.\n"
			"contents of packets as follows:\n%s\n", input.to_hex().c_str());
		return false;
	}

	dscp = ip_hdr->dscp_ecn >> 2;
	ecn = ip_hdr->dscp_ecn & 0x03;

	total_len = unpack_uint16(ip_hdr->total_len);
	if (total_len > input.size()-ethernet_hdr_len) {
		// ip packet length not valid
		output::log(output::loglevel::BUG, "ip_packet::deserialize() -- ip packet len invalid.\n"
			"indicated len: %u bytes, actual len: %u bytes, full len: %u bytes.\n",
			total_len,
			input.size()-ethernet_hdr_len,
			input.size());
		return false;
	}

	identification = unpack_uint16(ip_hdr->identification);
	if (identification != 0) {
		//	fragment id; ignore for now
	}
	
	// set the flags
	if ((ip_hdr->flags_frag_offset[0] >> 7) & 0x01) {
		// error! must be 0
		output::log(output::loglevel::BUG, "ip_packet::deserialize() -- fragment offset currently not supported.\n"
			"contents of packet as follows:\n%s\n", input.to_hex().c_str());
		return false;
	}

	dont_fragment = ((ip_hdr->flags_frag_offset[0] >> 6) & 0x01);
	more_fragments = ((ip_hdr->flags_frag_offset[0] >> 5) & 0x01);

	fragment_offset = (ip_hdr->flags_frag_offset[1] << 8) + (ip_hdr->flags_frag_offset[0] & 0x1f);
	ttl = ip_hdr->ttl;
	protocol = ip_hdr->protocol;
	ip_hdr_checksum = unpack_uint16(ip_hdr->header_checksum);
	src_ip.set_from_network_buffer(&ip_hdr->src_ip);
	dest_ip.set_from_network_buffer(&ip_hdr->dest_ip);

	if (ihl > 5) {
		ip_options.inherit_read_only(input.ptr_offset_const(ethernet_hdr_len+sizeof(ip_hdr_t)), (ihl-5)*sizeof(uint32_t));
	} else {
		ip_options.reset();
	}
	ip_payload.inherit_read_only(input.ptr_offset_const(ethernet_hdr_len+ihl*sizeof(uint32_t)), total_len-ihl*sizeof(uint32_t));

	return true;
}

uint16_t ip_packet::checksum_ip(const ip_hdr_t* hdr) {
	uint32_t checksum = 0;
	uint32_t iterations = sizeof(ip_hdr_t)/sizeof(uint16_t);
	uint16_t result = 0;

	if (hdr->header_checksum[0] != 0 || hdr->header_checksum[1] != 0) {
		ip_hdr_t ip_hdr;
		memcpy(&ip_hdr, hdr, sizeof(ip_hdr_t));
		ip_hdr.header_checksum[0] = 0;
		ip_hdr.header_checksum[1] = 0;

		for (uint32_t blocks = 0; blocks < iterations; blocks++) {
			checksum += unpack_uint16(&(((uint16_t*)&ip_hdr)[blocks]));
		}
	} else {
		for (uint32_t blocks = 0; blocks < iterations; blocks++) {
			checksum += unpack_uint16(&(((uint16_t*)hdr)[blocks]));
		}
	}

	// convert to ones complement
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = ~(checksum & 0xffff);

	return result;
}

// tcp packet
tcp_packet::tcp_packet() {
	packet_type = TCP_PACKET;
	protocol = 6;
}

// virtual destructor
tcp_packet::~tcp_packet() {
}

// clears the tcp packet
void tcp_packet::clear() {
	ip_packet::clear();
	packet_type = TCP_PACKET;
	protocol = 6;

	src_port = 0;
	dest_port = 0;
	seq = 0;
	ack = 0;

	ns_flag = false;
	cwr_flag = false;
	ece_flag = false;
	urg_flag = false;
	ack_flag = false;
	psh_flag = false;
	rst_flag = false;
	syn_flag = false;
	fin_flag = false;

	window_size = 14600;
	tcp_checksum = 0;
	urgent_ptr = 0;

	options.clear();
	payload.clear();
}

// generates a readable form of the TCP packet
string tcp_packet::to_string() const {
	char buf[16];
	string result = ip_packet::to_string();
	result += "\nsrc port        : ";
	sprintf(buf, "%d", src_port);
	result += buf;
	result += "\ndest port       : ";
	sprintf(buf, "%d", dest_port);
	result += buf;
	result += "\nseq             : ";
	sprintf(buf, "%u", seq);
	result += buf;
	result += "\nack             : ";
	sprintf(buf, "%u", ack);
	result += buf;
	result += "\nflags           : ";

	result += (ns_flag ? "NS " : "");
	result += (cwr_flag ? "CWR " : "");
	result += (ece_flag ? "ECE " : "");
	result += (urg_flag ? "URG " : "");
	result += (ack_flag ? "ACK " : "");
	result += (psh_flag ? "PSH " : "");
	result += (rst_flag ? "RST " : "");
	result += (syn_flag ? "SYN " : "");
	result += (fin_flag ? "FIN " : "");

	sprintf(buf, "%u", tcp_checksum);
	result += "\nchecksum        : ";
	result += buf;

	result += "\nwindow size     : ";
	sprintf(buf, "%d", window_size);
	result += buf;
	result += " bytes\noptions size    : ";
	sprintf(buf, "%d", options.size());
	result += buf;
	result += " bytes\npayload size    : ";
	sprintf(buf, "%d", payload.size());
	result += buf;
	result += " bytes";

	return result;
}

// serializes the tcp packet
// note: currently ignores URG flag
uint32_t tcp_packet::serialize(autobuf& dest) const {

	// ip packet serialization already reserves the full packet size and includes space for the
	// TCP header, options and TCP payload
	uint32_t tcp_size = sizeof(tcp_hdr_t)+options.size()+payload.size();
	uint32_t full_size = ip_packet::serialize_special(dest, tcp_size);
	tcp_hdr_t* tcp_hdr = (tcp_hdr_t*)dest.ptr_offset_mutable(full_size-tcp_size);
	uint16_t checksum;
	pack_uint16(tcp_hdr->src_port, src_port);
	pack_uint16(tcp_hdr->dest_port, dest_port);
	pack_uint32(tcp_hdr->seq, seq);
	pack_uint32(tcp_hdr->ack, ack);
	tcp_hdr->data_offset_reserved = ((5+(options.size() >> 2))<< 4) | (ns_flag ? 1 : 0);
	tcp_hdr->flags = (cwr_flag ? 1 << 7 : 0) | (ece_flag ? 1 << 6 : 0) | (ack_flag ? 1 << 4 : 0) | (psh_flag ? 1 << 3 : 0) | (rst_flag ? 1 << 2 : 0) | (syn_flag ? 1 << 1 : 0) | (fin_flag ? 1 : 0);
	pack_uint16(tcp_hdr->window_size, window_size);
	tcp_hdr->checksum[0] = 0;
	tcp_hdr->checksum[1] = 0;
	tcp_hdr->urgent_ptr[0] = 0;
	tcp_hdr->urgent_ptr[1] = 0;

	// merge tcp options and payload in
	if (options.size() > 0) {
		memcpy(tcp_hdr->data, options.get_content_ptr(), options.size());
	}

	if (payload.size() > 0) {
		memcpy(tcp_hdr->data+options.size(), payload.get_content_ptr(), payload.size());
	}

	uint32_t ethernet_hdr_len = (has_vlan_tag ? sizeof(tagged_ethernet_hdr_t) : sizeof(untagged_ethernet_hdr_t));

	autobuf tcp_options_and_payload((const void*)tcp_hdr->data, options.size()+payload.size());
	checksum = checksum_tcp((ip_hdr_t*)dest.ptr_offset_const(ethernet_hdr_len), tcp_hdr, tcp_options_and_payload);
	pack_uint16((uint8_t*)dest.get_content_ptr_mutable() + ethernet_hdr_len + 36, checksum);

	return full_size;
}

// deserializes the tcp packet
bool tcp_packet::deserialize(const autobuf& input) {
	if (!ip_packet::deserialize(input)) {
		return false;
	}

	if (ip_payload.size() < sizeof(tcp_hdr_t)) {
			output::log(output::loglevel::BUG, "tcp_packet::deserialize() -- ip payload size incorrect.\n"
				"expected at least %u bytes, actual: %u bytes.\n"
				"contents of packets as follows:\n%s\n",
				sizeof(tcp_hdr_t),
				ip_payload.size(),
				input.to_hex().c_str());
		return false;
	}

	// deserialize TCP packet
	const tcp_hdr_t* tcp_hdr = (const tcp_hdr_t*)ip_payload.get_content_ptr();

	src_port = unpack_uint16(tcp_hdr->src_port);
	dest_port = unpack_uint16(tcp_hdr->dest_port);
	seq = unpack_uint32(tcp_hdr->seq);
	ack = unpack_uint32(tcp_hdr->ack);
	uint32_t data_offset = tcp_hdr->data_offset_reserved >> 4;

	ns_flag = (tcp_hdr->data_offset_reserved & 0x01) ? true : false;
	cwr_flag = (tcp_hdr->flags & 0x80) ? true : false;
	ece_flag = (tcp_hdr->flags & 0x40) ? true : false;
	urg_flag = (tcp_hdr->flags & 0x20) ? true : false;
	ack_flag = (tcp_hdr->flags & 0x10) ? true : false;
	psh_flag = (tcp_hdr->flags & 0x08) ? true : false;
	rst_flag = (tcp_hdr->flags & 0x04) ? true : false;
	syn_flag = (tcp_hdr->flags & 0x02) ? true : false;
	fin_flag = (tcp_hdr->flags & 0x01) ? true : false;

	window_size = unpack_uint16(tcp_hdr->window_size);
	tcp_checksum = unpack_uint16(tcp_hdr->checksum);
	urgent_ptr = unpack_uint16(tcp_hdr->urgent_ptr);
	if (data_offset > 5) {
		options.inherit_read_only(((const autobuf&)ip_payload).ptr_offset_const(sizeof(tcp_hdr)), (data_offset-5)*sizeof(uint32_t));
	} else {
		options.reset();
	}

	if (ip_payload.size() > data_offset*sizeof(uint32_t)) {
		payload.inherit_read_only(((const autobuf&) ip_payload).ptr_offset_const(data_offset*sizeof(uint32_t)), ip_payload.size()-options.size()-sizeof(tcp_hdr_t));
	} else {

	}

	// TODO -- assert checksum
	return true;
}

// calculates the tcp cheksum
uint16_t tcp_packet::checksum_tcp(const ip_hdr_t* ip_hdr, const tcp_hdr_t* tcp_hdr, const autobuf& tcp_payload) {

	tcp_checksum_hdr_t checksum_hdr;
	uint32_t counter = 0;
	uint32_t checksum = 0;
	uint16_t result = 0;
	uint32_t iterations = sizeof(checksum_hdr) / sizeof(uint16_t);

	memcpy(checksum_hdr.src_ip, ip_hdr->src_ip, 8);	// copies both src and dest IP
	checksum_hdr.reserved = 0;
	checksum_hdr.protocol = 6;

	pack_uint16(checksum_hdr.tcp_segment_len, sizeof(tcp_hdr_t)+tcp_payload.size());

	memcpy(&checksum_hdr.tcp_hdr, tcp_hdr, sizeof(tcp_hdr_t));
	checksum_hdr.tcp_hdr.checksum[0] = 0;
	checksum_hdr.tcp_hdr.checksum[1] = 0;

	// compute first part of checksum
	for (counter = 0; counter < iterations; ++counter) {
		checksum += unpack_uint16(((uint16_t*)(&checksum_hdr))+counter);
	}

	// compute checksum for payload
	iterations = tcp_payload.size() / sizeof(uint16_t);
	for (counter = 0; counter < iterations; ++counter) {
		checksum += unpack_uint16(((uint16_t*)(tcp_payload.get_content_ptr()))+counter);
	}

	// account for odd sized payloads
	if (tcp_payload.size() % 2 == 1) {
		checksum += tcp_payload[tcp_payload.size()-1];// << 8;
	}

	// convert to ones complement
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);

	result = ~(checksum & 0xffff);

	return result;
}

// udp packet
udp_packet::udp_packet() {
	packet_type = UDP_PACKET;
	protocol = 17;
}

// virtual destructor
udp_packet::~udp_packet() {
}

// clears the UDP packet
void udp_packet::clear() {
	ip_packet::clear();
	packet_type = UDP_PACKET;
	src_port = 0;
	dest_port = 0;
	udp_checksum = 0;
	udp_payload.clear();
	protocol = 17;	// UDP protocol number
}

// generates a readable form of the UDP packet
string udp_packet::to_string() const {
	char buf[16];
	string result = ip_packet::to_string();
	result += "\nsrc port        : ";
	sprintf(buf, "%u", src_port);
	result += buf;
	result += "\ndest port       : ";
	sprintf(buf, "%u", dest_port);
	result += buf;
	result += "\nudp checksum    : ";
	sprintf(buf, "%u", udp_checksum);
	result += buf;
	result += "\nudp payload size: ";
	sprintf(buf, "%u", udp_payload.size());
	result += buf;

	return result;
}

// serializes the UDP packet
uint32_t udp_packet::serialize(autobuf& dest) const {

	uint32_t udp_size = sizeof(udp_hdr_t) + udp_payload.size();
	uint32_t full_size = ip_packet::serialize_special(dest, udp_size);
	udp_hdr_t* udp_hdr = (udp_hdr_t*)dest.ptr_offset_mutable(full_size-udp_size);
	
	// generate the ip payload
	pack_uint16(udp_hdr->src_port, src_port);
	pack_uint16(udp_hdr->dest_port, dest_port);
	pack_uint16(udp_hdr->len, sizeof(udp_hdr_t)+udp_payload.size());
	pack_uint16(udp_hdr->checksum, 0);

	// TODO -- calculate an actual, valid checksum

	// copy udp header and payload in
	memcpy(udp_hdr->data, udp_payload.get_content_ptr(), udp_payload.size());

	return full_size;
}

// deserializes a UDP packet
bool udp_packet::deserialize(const autobuf& input) {
	if (!ip_packet::deserialize(input)) {
		output::log(output::loglevel::BUG, "udp_packet::deserialize() -- ip header deserialization failed.\n");
		return false;
	}

	if (ip_payload.size() < sizeof(udp_hdr_t)) {
		output::log(output::loglevel::BUG, "udp_packet::deserialize() -- ip header size incorrect.\n"
			"expected at least %u bytes, actual: %u bytes.\n"
			"contents of packet as follows:\n%s\n",
			sizeof(udp_hdr_t),
			ip_payload.size(),
			input.to_hex().c_str());
		return false;
	}

	const udp_hdr_t* udp_hdr = (const udp_hdr_t*)ip_payload.get_content_ptr();
	
	src_port = unpack_uint16(udp_hdr->src_port);
	dest_port = unpack_uint16(udp_hdr->dest_port);
	udp_checksum = unpack_uint16(udp_hdr->checksum);

	uint16_t udp_hdr_len = unpack_uint16(udp_hdr->len);
	if (udp_hdr_len != ip_payload.size()) {
		output::log(output::loglevel::BUG, "udp_packet::deserialize() -- udp length field incorrect.\n"
			"expected %u bytes, actual: % bytes.\n"
			"contents of packet as follows:\n%s\n",
			ip_payload.size(),
			udp_hdr_len,
			input.to_hex().c_str());
		return false;
	}

	if (ip_payload.size() > sizeof(udp_hdr))
		udp_payload.inherit_read_only(((const autobuf&)ip_payload).ptr_offset_const(sizeof(udp_hdr_t)), ip_payload.size()-sizeof(udp_hdr_t));

	return true;
}

uint16_t udp_packet::checksum_udp(const ip_hdr_t* ip_hdr, const udp_hdr_t* udp_hdr, const autobuf& udp_payload) {
	udp_checksum_hdr_t checksum_hdr;
	uint32_t counter = 0;
	uint32_t checksum = 0;
	uint16_t result = 0;
	uint32_t iterations = sizeof(checksum_hdr) / sizeof(uint16_t);

	memcpy(checksum_hdr.src_ip, ip_hdr->src_ip, 8);	// copies both src and dest IP
	checksum_hdr.reserved = 0;
	checksum_hdr.protocol = 17;
	pack_uint16(checksum_hdr.len, sizeof(udp_checksum_hdr_t)+udp_payload.size());
	memcpy(&checksum_hdr.udp_hdr, udp_hdr, sizeof(udp_hdr_t));
	checksum_hdr.udp_hdr.checksum[0] = 0;
	checksum_hdr.udp_hdr.checksum[1] = 0;

	// compute first part of checksum
	for (counter = 0; counter < iterations; ++counter) {
		checksum += ((uint16_t*)&checksum_hdr)[counter];
	}

	// compute checksum for payload
	iterations = udp_payload.size() / sizeof(uint16_t);
	for (uint32_t counter = 0; counter < iterations; ++counter) {
		checksum += ((uint16_t*) udp_payload.get_content_ptr())[counter];
	}

	// account for odd sized payloads
	if (udp_payload.size() % 2 == 1) {
		checksum += udp_payload[udp_payload.size()-1] << 8;
	}

	// convert to ones complement
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = ~(checksum & 0xffff);

	return result;
}

// dhcp constructor
dhcp_packet::dhcp_packet() {
	packet_type = DHCP_PACKET;
	hardware_type = 0;
	hw_addr_len = 0;
}

// virtual destructor
dhcp_packet::~dhcp_packet() {
}

// clears the dhcp packet
void dhcp_packet::clear() {
	request = false;
	hardware_type = 0;
	hw_addr_len = 0;
	hops = 0;
	xid = 0;
	seconds = 0;
	is_broadcast = false;
	packet_type = DHCP_PACKET;
	client_address.clear();
	your_address.clear();
	server_address.clear();
	gateway_address.clear();
	client_hw_address.clear();
	dhcp_options.clear();
}

// generates a readable version of the packet
string dhcp_packet::to_string() const {
/*	char buf[128];
	string result;
	result = "DHCP ";
	result += (request ? "REQUEST" : "REPLY");
	result += "src ";
	result += src_ip;
	result += "dest ";
	result += dest_ip;
*/

	// TODO -- implement this
	return string();
}

// serializes a dhcp packet
uint32_t dhcp_packet::serialize(autobuf& dest) const {

	output::log(output::loglevel::BUG, "dhcp_packet::serialize() -- not implemented.\n");
	abort();
	return 0;

	/*
	dhcp_hdr_t dhcp_hdr;
	memset(&dhcp_hdr, 0, sizeof(dhcp_hdr));
	dhcp_hdr.op = (request ? 1 : 2);
	dhcp_hdr.htype = 1;
	dhcp_hdr.hlen = 6;
	dhcp_hdr.hops = 0;
	pack_uint32(dhcp_hdr.xid, xid);
	pack_uint16(dhcp_hdr.secs, seconds);
	pack_uint16(dhcp_hdr.flags, (is_broadcast ? 1 : 0));
	client_address.get_address_network_order(&dhcp_hdr.client_addr);
	your_address.get_address_network_order(&dhcp_hdr.your_addr);
	server_address.get_address_network_order(&dhcp_hdr.server_addr);
	gateway_address.get_address_network_order(&dhcp_hdr.gateway_addr);

//	pack_uint32(dhcp_hdr.client_addr, client_address.get_uint32());
//	pack_uint32(dhcp_hdr.your_addr, your_address.get_uint32());
//	pack_uint32(dhcp_hdr.server_addr, server_address.get_uint32());
//	pack_uint32(dhcp_hdr.gateway_addr, gateway_address.get_uint32());
	client_hw_address.get(dhcp_hdr.client_hwaddr);
	pack_uint32(dhcp_hdr.magic_cookie, 0x63825363);
	udp_payload = autobuf(&dhcp_hdr, sizeof(dhcp_hdr)) + dhcp_options;
	return udp_packet::serialize();
	*/
}

// deserializes a dhcp packet
bool dhcp_packet::deserialize(const autobuf& input) {

	output::log(output::loglevel::BUG, "dhcp_packet::deserialize() error -- not implemented.\n");
	abort();
	return false;

	/*
	clear();
	if (!udp_packet::deserialize(input)) {
		return false;
	}
		printf(" deserializing dhcp \n");
	dhcp_hdr_t dhcp_hdr;
	if (udp_payload.size() < sizeof(dhcp_hdr)) {
		return false;
	}
		printf("	here \n");
	uint8_t buf[192];
	memset(buf, 0, sizeof(buf));
	memcpy(&dhcp_hdr, udp_payload.get_content_ptr(), sizeof(dhcp_hdr));
	if (udp_payload.size() - sizeof(dhcp_hdr) > 0) {
		udp_payload.read_arbitrary(dhcp_options, sizeof(dhcp_hdr), udp_payload.size()-sizeof(dhcp_hdr));
	}
		printf(" here 2 \n");
	if (dhcp_hdr.op == 1) {
		request = true;
	} else if (dhcp_hdr.op == 2) {
		request = false;
	}	else {
		goto fail;
	}

	if (dhcp_hdr.htype == 1 && dhcp_hdr.hlen == 6) {
		hardware_type = 1;
		hw_addr_len = 6;
	} else {
		// not supported
		goto fail;
	}
		printf("here 3\n");
	hops = dhcp_hdr.hops;
	xid = unpack_uint32(dhcp_hdr.xid);
	seconds = unpack_uint16(dhcp_hdr.secs);
	if (unpack_uint16(dhcp_hdr.flags) == 1) {
		is_broadcast = true;
	} else {
		is_broadcast = false;
	}
		printf("here 4\n");
	client_address.set_from_network_buffer(&dhcp_hdr.client_addr);
	your_address.set_from_network_buffer(&dhcp_hdr.your_addr);
	server_address.set_from_network_buffer(&dhcp_hdr.server_addr);
	gateway_address.set_from_network_buffer(&dhcp_hdr.gateway_addr);

//	client_address.set(unpack_uint32(dhcp_hdr.client_addr));
//	your_address.set(unpack_uint32(dhcp_hdr.your_addr));
//	server_address.set(unpack_uint32(dhcp_hdr.server_addr));
//	gateway_address.set(unpack_uint32(dhcp_hdr.gateway_addr));

	client_hw_address.set_from_network_buffer(dhcp_hdr.client_hwaddr);

	// validate bootp field and magic cookie
	if (memcmp(dhcp_hdr.bootp_fields, buf, sizeof(buf)) != 0
		|| unpack_uint32(dhcp_hdr.magic_cookie) != 0x63825363) {
		printf("bootp/magic cookie field incorrect.\n");
		goto fail;
	}
		printf("here 5\n");
	return true;

fail:
	clear();
	return false;
	*/
}

// constructor
icmp_packet::icmp_packet() {
	packet_type = ICMP_PACKET;
	protocol = 1;
}

// virtual destructor
icmp_packet::~icmp_packet() {
}

// clears the packet
void icmp_packet::clear() {
	ip_packet::clear();
	protocol = 1;
	packet_type = ICMP_PACKET;
	icmp_pkt_type = 0;
	code = 0;
	checksum = 0;
	icmp_hdr_rest = 0;

	data.clear();
}

// generates a readable version
string icmp_packet::to_string() const {
	char buf[10];
	string result = ip_packet::to_string();
	result += "\nICMP packet type: ";
	switch (icmp_pkt_type) {
		case 0:
			result += "echo reply";
			break;

		case 1:
		case 2:
			result += "reserved";
			break;

		case 3:
			result += "destination unreachable";
			break;

		case 8:
			result += "echo request";
			break;

		default:
			result += "other";
	}

	sprintf(buf, "%u", code);
	result += string("\ncode: ") + string(buf);

	return result;
}

// serializes the ICMP packet
uint32_t icmp_packet::serialize(autobuf& dest) const {

	uint32_t icmp_size = sizeof(icmp_hdr_t) + data.size();
	uint32_t full_size = ip_packet::serialize_special(dest, icmp_size);
	icmp_hdr_t* icmp_hdr = (icmp_hdr_t*) dest.ptr_offset_mutable(full_size-icmp_size);// sizeof(icmp_hdr_t));
	uint16_t checksum;

	icmp_hdr->type = icmp_pkt_type;
	icmp_hdr->code = code;
	icmp_hdr->checksum[0] = 0;
	icmp_hdr->checksum[1] = 0;
	pack_uint32(icmp_hdr->rest, icmp_hdr_rest);

	// copy in icmp data
	if (data.size() > 0) {
		memcpy(icmp_hdr->data, data.get_content_ptr(), data.size());
	}

	checksum = checksum_icmp(icmp_hdr, icmp_size);
	dest[dest.size()-data.size()-sizeof(uint32_t)-sizeof(uint16_t)] = checksum & 0x00ff;
	dest[dest.size()-data.size()-sizeof(uint32_t)-sizeof(uint16_t)+1] = (checksum & 0xff00) >> 8;

	return full_size;
}

// deserializes an ICMP packet
bool icmp_packet::deserialize(const autobuf& input) {
	if (!ip_packet::deserialize(input)) {
		output::log(output::loglevel::BUG, "icmp_packet::deserialize() -- ip header deserialization failed.\n");
		return false;
	}

	if (ip_payload.size() < sizeof(icmp_hdr_t)) {
		output::log(output::loglevel::BUG, "icmp_packet::deserialize() -- header size incorrect.\n"
			"expected %u bytes, actual: %u bytes.\n"
			"contents of packet as follows:\n%s\n",
			sizeof(icmp_hdr_t),
			ip_payload.size(),
			input.to_hex().c_str());
		return false;
	}

	const icmp_hdr_t* icmp_hdr = (const icmp_hdr_t*) ip_payload.get_content_ptr();
	if (input.size() < sizeof(icmp_hdr)) {
		return false;
	}

	icmp_pkt_type = icmp_hdr->type;
	code = icmp_hdr->code;
	checksum = unpack_uint16(icmp_hdr->checksum);
	icmp_hdr_rest = unpack_uint32(icmp_hdr->rest);

	if (ip_payload.size() > sizeof(icmp_hdr)) {
		data.inherit_read_only(((const autobuf&)ip_payload).ptr_offset_const(sizeof(icmp_hdr_t)), ip_payload.size()-sizeof(icmp_hdr_t));
	} else {
		data.reset();
	}

	return true;
}

// computes the icmp checksum
uint16_t icmp_packet::checksum_icmp(const icmp_hdr_t* icmp_hdr, uint32_t size) {
	*((uint16_t*)icmp_hdr->checksum) = 0;
	uint32_t iterations = size/sizeof(uint16_t);
	uint32_t checksum=0;

	for (uint32_t counter = 0; counter < iterations; ++counter) {
		checksum += ((uint16_t*) icmp_hdr)[counter];
	}

	// account for odd-sized payloads
	if (size & 0x01) {
		checksum += ((uint8_t*)icmp_hdr)[size-1] << 8;
	}

	// convert to ones complement
	uint16_t result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = checksum & 0xffff;
	checksum = result + ((checksum & 0xffff0000) >> 16);
	result = ~(checksum & 0xffff);

	return result;
}


// adds/modifies a vlan tag for a raw packet
bool std_packet_utils::set_vlan_tag(const autobuf& input, autobuf& output, uint16_t vlan) {

	if (input.size() < sizeof(raw_packet::untagged_ethernet_hdr_t)) {
		output.clear();
		return false;
	}

	const raw_packet::tagged_ethernet_hdr_t* tagged_src_hdr = (const raw_packet::tagged_ethernet_hdr_t*) input.get_content_ptr();
	if (unpack_uint16(tagged_src_hdr->vlan_8100) == 0x8100) {

		// this packet was already tagged. copy the packet and change the tagging
		output.full_copy_from(input);
		raw_packet::tagged_ethernet_hdr_t* tagged_hdr = (raw_packet::tagged_ethernet_hdr_t*) output.get_content_ptr_mutable();
		tagged_hdr->vlan_pcp_dei_id[0] &= 0xf0;
		tagged_hdr->vlan_pcp_dei_id[1] = 0;
		tagged_hdr->vlan_pcp_dei_id[0] |= ((vlan >> 8) & 0x0f);
		tagged_hdr->vlan_pcp_dei_id[1] = (vlan & 0x00ff);

	} else {

		const raw_packet::untagged_ethernet_hdr_t* untagged_src_hdr = (const raw_packet::untagged_ethernet_hdr_t*) input.get_content_ptr();

		// packet was untagged and we need to insert the tagged header
		output.create_empty_buffer(input.size() + sizeof(raw_packet::tagged_ethernet_hdr_t) - sizeof(raw_packet::untagged_ethernet_hdr_t), false);
		raw_packet::tagged_ethernet_hdr_t* dest_hdr = (raw_packet::tagged_ethernet_hdr_t*) output.get_content_ptr_mutable();
		memcpy(dest_hdr, untagged_src_hdr, 12);
		dest_hdr->vlan_8100[0] = 0x81;
		dest_hdr->vlan_8100[1] = 0x00;
		dest_hdr->vlan_pcp_dei_id[0] = ((vlan >> 8) & 0x0f);
		dest_hdr->vlan_pcp_dei_id[1] = (vlan & 0x00ff);
		dest_hdr->ethertype[0] = untagged_src_hdr->ethertype[0];
		dest_hdr->ethertype[1] = untagged_src_hdr->ethertype[1];
		memcpy(output.ptr_offset_mutable(sizeof(raw_packet::tagged_ethernet_hdr_t)),
			input.ptr_offset_const(sizeof(raw_packet::untagged_ethernet_hdr_t)),
			input.size()-sizeof(raw_packet::untagged_ethernet_hdr_t));
	}

	return true;
}

// removes the vlan tag from a raw packet
bool std_packet_utils::strip_vlan_tag(const autobuf& input, autobuf& output) {

	if (input.size() < sizeof(raw_packet::untagged_ethernet_hdr_t)) {
		output.clear();
		return false;
	}

	const raw_packet::tagged_ethernet_hdr_t* src_hdr = (const raw_packet::tagged_ethernet_hdr_t*) input.get_content_ptr();
	if (unpack_uint16(src_hdr->ethertype) != 0x8100) {

		// packet is already untagged
		output.full_copy_from(input);

	} else {

		output.create_empty_buffer(input.size() - sizeof(raw_packet::tagged_ethernet_hdr_t) - sizeof(raw_packet::untagged_ethernet_hdr_t), false);
		raw_packet::untagged_ethernet_hdr_t* dest_hdr = (raw_packet::untagged_ethernet_hdr_t*) output.get_content_ptr_mutable();

		memcpy(dest_hdr, src_hdr, 12);
		memcpy(dest_hdr->ethertype, src_hdr->ethertype, 2);
		memcpy(output.ptr_offset_mutable(sizeof(raw_packet::untagged_ethernet_hdr_t)),\
			input.ptr_offset_const(sizeof(raw_packet::tagged_ethernet_hdr_t)),
			input.size()-sizeof(raw_packet::tagged_ethernet_hdr_t));
	}

	return true;
}

// creates the appropriate packet type from the data
// packet is dynamically-allocated
raw_packet* std_packet_factory::instantiate(const autobuf& data) {
	raw_packet* result = nullptr;
	
	raw_packet type_finder_pkt;
	if (!type_finder_pkt.deserialize(data)) {
		output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- raw packet deserialization failed.\n");
		return nullptr;
	}

	// switch based on the type to instantiate
	switch (type_finder_pkt.packet_type) {
		case raw_packet::ARP_PACKET:
			result = new arp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- ARP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::ICMP_PACKET:
			result = new icmp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- ICMP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::TCP_PACKET:
			result = new tcp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- TCP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::UDP_PACKET:
			result = new udp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- UDP packet deserialization error.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::IPV6_PACKET:
			// no-op for now
			result = nullptr;
			break;

		default:
			output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- unsupported packet type %d.\n", type_finder_pkt.packet_type);
			result = nullptr;
	}

	return result;
}

// creates the appropriate packet type from the data
// using a preallocated buffer
raw_packet* std_packet_factory::instantiate(autobuf* msg_buf, const autobuf& data) {
	raw_packet* result = nullptr;
	
	raw_packet type_finder_pkt;
	if (!type_finder_pkt.deserialize(data)) {
		output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- raw packet deserialization failed.\n");
		return nullptr;
	}

	// switch based on the type to instantiate
	switch (type_finder_pkt.packet_type) {
		case raw_packet::ARP_PACKET:
			result = new (msg_buf->get_content_ptr_mutable()) arp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- ARP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::ICMP_PACKET:
			result = new (msg_buf->get_content_ptr_mutable()) icmp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- ICMP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::TCP_PACKET:
			result = new (msg_buf->get_content_ptr_mutable()) tcp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- TCP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::UDP_PACKET:
			result = new (msg_buf->get_content_ptr_mutable()) udp_packet();
			if (result == nullptr || !result->deserialize(data)) {
				output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- UDP packet deserialization failed.\n");
				delete result;
				result = nullptr;
			}
			break;

		case raw_packet::IPV6_PACKET:
			// no-op for now
			result = nullptr;
			break;

		default:
			output::log(output::loglevel::BUG, "std_packet_factory::instantiate() -- unsupported packet type %d.\n", type_finder_pkt.packet_type);
			result = nullptr;
	}

	return result;
}
