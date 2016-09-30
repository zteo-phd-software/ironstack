#include "of_action.h"
#include "../gui/output.h"

// checks if two of_actions (given as base classes) are equivalent. useful for action deduplication
bool of_action::operator==(const of_action& other) const {

	if ((type != other.type) || (type == action_type::UNKNOWN || other.type == action_type::UNKNOWN)) {
		return false;
	}

	switch (other.type) {
	case action_type::OUTPUT_TO_PORT:
		return *((of_action_output_to_port*) this) == *((of_action_output_to_port*) &other);

	case action_type::ENQUEUE:
		return *((of_action_enqueue*) this) == *((of_action_enqueue*) &other);

	case action_type::SET_VLAN_ID:
		return *((of_action_set_vlan_id*) this) == *((of_action_set_vlan_id*) &other);

	case action_type::SET_VLAN_PCP:
		return *((of_action_set_vlan_pcp*) this) == *((of_action_set_vlan_pcp*) &other);

	case action_type::STRIP_VLAN:
		return true;

	case action_type::SET_DL_ADDR:
		return *((of_action_set_mac_address*) this) == *((of_action_set_mac_address*) &other);

	case action_type::SET_NW_ADDR:
		return *((of_action_set_ip_address*) this) == *((of_action_set_ip_address*) &other);

	case action_type::SET_IP_TOS:
		return *((of_action_set_ip_type_of_service*) this) == *((of_action_set_ip_type_of_service*) &other);

	case action_type::SET_TCPUDP_PORT:
		return *((of_action_set_tcpudp_port*) this) == *((of_action_set_tcpudp_port*) &other);

	case action_type::VENDOR:
		return *((of_action_vendor*) this) == *((of_action_vendor*) &other);

	default:
		return false;
	}

	return true;
}

// used to clone the derived class given just the base class ptr
of_action* of_action::clone(const of_action* original) {

	switch (original->type) {
	case action_type::OUTPUT_TO_PORT:
		return new of_action_output_to_port(*static_cast<const of_action_output_to_port*>(original));
	
	case action_type::ENQUEUE:
		return new of_action_enqueue(*static_cast<const of_action_enqueue*>(original));
	
	case action_type::SET_VLAN_ID:
		return new of_action_set_vlan_id(*static_cast<const of_action_set_vlan_id*>(original));

	case action_type::SET_VLAN_PCP:
		return new of_action_set_vlan_pcp(*static_cast<const of_action_set_vlan_pcp*>(original));

	case action_type::STRIP_VLAN:
		return new of_action_strip_vlan(*static_cast<const of_action_strip_vlan*>(original));

	case action_type::SET_DL_ADDR:
		return new of_action_set_mac_address(*static_cast<const of_action_set_mac_address*>(original));
	
	case action_type::SET_NW_ADDR:
		return new of_action_set_ip_address(*static_cast<const of_action_set_ip_address*>(original));

	case action_type::SET_IP_TOS:
		return new of_action_set_ip_type_of_service(*static_cast<const of_action_set_ip_type_of_service*>(original));
	
	case action_type::SET_TCPUDP_PORT:
		return new of_action_set_tcpudp_port(*static_cast<const of_action_set_tcpudp_port*>(original));
	
	case action_type::VENDOR:
		return new of_action_vendor(*static_cast<const of_action_vendor*>(original));

	default:
		return nullptr;
	}
}

// returns the message type of the packet so it can be forwarded for
// proper deserializaton
of_action::action_type of_action::infer_type(const autobuf& content) {

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(struct ofp_action_header)) {
		return action_type::UNKNOWN;
	}
	#endif

	uint16_t raw_type = *((uint16_t*)(content.get_content_ptr()));
	raw_type = ntohs(raw_type);

	switch (raw_type) {
		case (uint16_t) OFPAT_OUTPUT:
			return action_type::OUTPUT_TO_PORT;

		case (uint16_t) OFPAT_ENQUEUE:
			return action_type::ENQUEUE;

		case (uint16_t) OFPAT_SET_VLAN_VID:
			return action_type::SET_VLAN_ID;

		case (uint16_t) OFPAT_SET_VLAN_PCP:
			return action_type::SET_VLAN_PCP;

		case (uint16_t) OFPAT_STRIP_VLAN:
			return action_type::STRIP_VLAN;

		case (uint16_t) OFPAT_SET_DL_SRC:
		case (uint16_t) OFPAT_SET_DL_DST:
			return action_type::SET_DL_ADDR;

		case (uint16_t) OFPAT_SET_NW_SRC:
		case (uint16_t) OFPAT_SET_NW_DST:
			return action_type::SET_NW_ADDR;

		case (uint16_t) OFPAT_SET_NW_TOS:
			return action_type::SET_IP_TOS;

		case (uint16_t) OFPAT_SET_TP_SRC:
		case (uint16_t) OFPAT_SET_TP_DST:
			return action_type::SET_TCPUDP_PORT;

		case (uint16_t) OFPAT_VENDOR:
			return action_type::VENDOR;

		default:
			break;
	}

	return action_type::UNKNOWN;
}

// constructor for the action to output to a port
of_action_output_to_port::of_action_output_to_port() {
	clear();
}

// clears the output action
void of_action_output_to_port::clear() {
	type = action_type::OUTPUT_TO_PORT;
	send_to_controller = false;
	port = 0;
}

// compares two of_action_output_port
bool of_action_output_to_port::operator==(const of_action_output_to_port& other) const {
	return (send_to_controller == other.send_to_controller && port == other.port);
}

// generates a readable string for the output action
std::string of_action_output_to_port::to_string() const {
	char buf[16];
	std::string result = "out_port=";
	if (send_to_controller) {
		result += "controller";
	} else {
		sprintf(buf, "%hu", port);
		result += buf;
	}

	return result;
}

// generates the header for the output action
uint32_t of_action_output_to_port::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_output), false);
	struct ofp_action_output* hdr = (struct ofp_action_output*) dest.get_content_ptr_mutable();
	hdr->type = htons((uint16_t) OFPAT_OUTPUT);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_output));
	hdr->port = htons((uint16_t) send_to_controller ? OFPP_CONTROLLER : port);
	hdr->max_len = htons((uint16_t) 65535);

	return sizeof(struct ofp_action_output);
}

// deserializes the action
bool of_action_output_to_port::deserialize(const autobuf& content) {
	clear();

	const struct ofp_action_output* hdr = (const struct ofp_action_output*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_OUTPUT || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	port = ntohs(hdr->port);
	if (port == OFPP_CONTROLLER) {
		send_to_controller = true;
	} else {
		send_to_controller = false;
	}

	return true;
}

// returns the number of bytes that this deserialization consumes
uint32_t of_action_output_to_port::get_deserialization_size() const {
	return 8;
}

// constructor for the enqueue action
of_action_enqueue::of_action_enqueue() {
	clear();
}

// clears the enqueue action object
void of_action_enqueue::clear() {
	type = action_type::ENQUEUE;
	port = 0;
	queue_id = 0;
}

// checks for equality
bool of_action_enqueue::operator==(const of_action_enqueue& other) const {
	return (port == other.port && queue_id == other.queue_id);
}

// generates a readable string for the enqueue action
std::string of_action_enqueue::to_string() const {
	char buf[16];
	std::string result = "enqueue to ";
	sprintf(buf, "%d", port);
	result += buf;
	result += " queue id: ";
	sprintf(buf, "%d", queue_id);
	result += buf;

	return result;
}

// generates the header for the enqueue action
uint32_t of_action_enqueue::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_enqueue), false);
	struct ofp_action_enqueue* hdr = (struct ofp_action_enqueue*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) OFPAT_ENQUEUE);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_enqueue));
	hdr->port = htons(port);
	hdr->queue_id = htonl(queue_id);

	return sizeof(struct ofp_action_enqueue);
}

// deserializes the enqueue action from a message
bool of_action_enqueue::deserialize(const autobuf& content) {
	clear();

	const struct ofp_action_enqueue* hdr = (const struct ofp_action_enqueue*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_ENQUEUE || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	port = ntohs(hdr->port);
	queue_id = ntohl(hdr->queue_id);

	return true;
}

// returns size of serialized action
uint32_t of_action_enqueue::get_deserialization_size() const {
	return 16;
}

// constructor to set vlan id
of_action_set_vlan_id::of_action_set_vlan_id() {
	clear();
}

// clears the action to set the vlan id
void of_action_set_vlan_id::clear() {
	type = action_type::SET_VLAN_ID;
	vlan_id = 0;
}

// checks for equality
bool of_action_set_vlan_id::operator==(const of_action_set_vlan_id& other) const {
	return vlan_id == other.vlan_id;
}

// generates a readable form of the action
std::string of_action_set_vlan_id::to_string() const {
	char buf[16];
	std::string result = "set vlan id=";
	sprintf(buf, "%d", vlan_id);
	result += buf;

	return result;
}

// generates the header for the set vlan vid action
uint32_t of_action_set_vlan_id::serialize(autobuf& dest) const {
	dest.create_empty_buffer(sizeof(struct ofp_action_vlan_vid), false);
	struct ofp_action_vlan_vid* hdr = (struct ofp_action_vlan_vid*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) OFPAT_SET_VLAN_VID);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_vlan_vid));
	hdr->vlan_vid = htons(vlan_id);
	
	return sizeof(struct ofp_action_vlan_vid);
}

// deserializes the set vlan id action from an autobuf
bool of_action_set_vlan_id::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_vlan_vid* hdr = (const struct ofp_action_vlan_vid*) content.get_content_ptr();;

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_SET_VLAN_VID || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	vlan_id = ntohs(hdr->vlan_vid);
	return true;
}

// gets the size of the serialized action
uint32_t of_action_set_vlan_id::get_deserialization_size() const {
	return 8;
}

// constructor to set the vlan pcp
of_action_set_vlan_pcp::of_action_set_vlan_pcp() {
	clear();
}

// clears the set vlan pcp action object
void of_action_set_vlan_pcp::clear() {
	type = action_type::SET_VLAN_PCP;
	vlan_pcp = 0;
}

// checks for equality
bool of_action_set_vlan_pcp::operator==(const of_action_set_vlan_pcp& other) const {
	return vlan_pcp == other.vlan_pcp;
}

// generates a readable form of the action
std::string of_action_set_vlan_pcp::to_string() const {
	char buf[16];
	std::string result = "set vlan pcp=";
	sprintf(buf, "%d", vlan_pcp);
	result += buf;

	return result;
}

// generates the header for the set vlan pcp action
uint32_t of_action_set_vlan_pcp::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_vlan_pcp), false);
	struct ofp_action_vlan_pcp* hdr = (struct ofp_action_vlan_pcp*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) OFPAT_SET_VLAN_PCP);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_vlan_pcp));
	hdr->vlan_pcp = vlan_pcp;
	
	return sizeof(struct ofp_action_vlan_pcp);
}

// deserializes the set vlan pcp action from an autobuf
bool of_action_set_vlan_pcp::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_vlan_pcp* hdr = (const struct ofp_action_vlan_pcp*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_SET_VLAN_PCP || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	vlan_pcp = hdr->vlan_pcp;

	return true;
}

// returns the size of the set vlan pcp action
uint32_t of_action_set_vlan_pcp::get_deserialization_size() const {
	return 8;
}

// constructor to strip the vlan header
of_action_strip_vlan::of_action_strip_vlan() {
	clear();
}

// clears the strip vlan header object
void of_action_strip_vlan::clear() {
	type = action_type::STRIP_VLAN;
}

// generates a readable form of the action
std::string of_action_strip_vlan::to_string() const {
	return std::string("strip vlan header");
}

// generates the header for the action
uint32_t of_action_strip_vlan::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_header), false);
	struct ofp_action_header* hdr = (struct ofp_action_header*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) OFPAT_STRIP_VLAN);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_header));

	return sizeof(struct ofp_action_header);
}

// deserializes the strip vlan action from an autobuf
bool of_action_strip_vlan::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_header* hdr = (const struct ofp_action_header*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_STRIP_VLAN || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	return true;
}

// returns the size of the strip vlan action
uint32_t of_action_strip_vlan::get_deserialization_size() const {
	return 8;
}

// constructor for setting mac address
of_action_set_mac_address::of_action_set_mac_address() {
	clear();
}

// clears the action for setting the mac address
void of_action_set_mac_address::clear() {
	type = action_type::SET_DL_ADDR;
	set_src = true;
	new_addr.clear();
}

// checks for equality
bool of_action_set_mac_address::operator==(const of_action_set_mac_address& other) const {
	return set_src == other.set_src && new_addr == other.new_addr;
}

// generates a readable form of the action
std::string of_action_set_mac_address::to_string() const {
	return std::string(set_src ? "set dl_src=" : "set dl_dest=") + new_addr.to_string();
}

// generates the action header
uint32_t of_action_set_mac_address::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_dl_addr), false);
	struct ofp_action_dl_addr* hdr = (struct ofp_action_dl_addr*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) set_src ? OFPAT_SET_DL_SRC : OFPAT_SET_DL_DST);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_dl_addr));
	new_addr.get(hdr->dl_addr);

	return sizeof(struct ofp_action_dl_addr);
}

// deserializes the set dl src/dst action from an autobuf
bool of_action_set_mac_address::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_dl_addr* hdr = (const struct ofp_action_dl_addr*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);
	
	if ((hdr_type != (uint16_t) OFPAT_SET_DL_SRC && hdr_type != (uint16_t) OFPAT_SET_DL_DST)
		|| hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	set_src = (ntohs(hdr->type) == (uint16_t) OFPAT_SET_DL_SRC);
	new_addr.set_from_network_buffer(hdr->dl_addr);

	return true;
}

// returns the size of the set dl src/dst action
uint32_t of_action_set_mac_address::get_deserialization_size() const {
	return 16;
}

// constructor for action to set ip address
of_action_set_ip_address::of_action_set_ip_address() {
	clear();
}

// clears the action to setting the ip address
void of_action_set_ip_address::clear() {
	type = action_type::SET_NW_ADDR;
	set_src = true;
	new_addr.clear();
}

// checks for equality
bool of_action_set_ip_address::operator==(const of_action_set_ip_address& other) const {
	return set_src == other.set_src && new_addr == other.new_addr;
}

// generates a readable form of the action
std::string of_action_set_ip_address::to_string() const {
	return std::string(set_src ? "set nw_src=" : "set nw_dest=") + new_addr.to_string();
}

// generates the action header
uint32_t of_action_set_ip_address::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_nw_addr), false);
	struct ofp_action_nw_addr* hdr = (struct ofp_action_nw_addr*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) set_src ? OFPAT_SET_NW_SRC : OFPAT_SET_NW_DST);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_nw_addr));
	new_addr.get_address_network_order(&hdr->nw_addr);

	return sizeof(struct ofp_action_nw_addr);
}

// deserializes the set nw addr action from an autobuf
bool of_action_set_ip_address::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_nw_addr* hdr = (const struct ofp_action_nw_addr*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != ((uint16_t) OFPAT_SET_NW_SRC && hdr_type != (uint16_t) OFPAT_SET_NW_DST)
		|| hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	set_src = (ntohs(hdr->type) == (uint16_t) OFPAT_SET_NW_SRC);
	uint32_t raw_address = ntohl(hdr->nw_addr);
	new_addr.set_from_network_buffer(&raw_address);

	return true;
}

// returns the size of the set nw addr action
uint32_t of_action_set_ip_address::get_deserialization_size() const {
	return 8;
}

// constructor to set the ip type of service
of_action_set_ip_type_of_service::of_action_set_ip_type_of_service() {
	clear();
}

// clears the object
void of_action_set_ip_type_of_service::clear() {
	type = action_type::SET_IP_TOS;
	type_of_service = 0;
}

// checks for equality
bool of_action_set_ip_type_of_service::operator==(const of_action_set_ip_type_of_service& other) const {
	return type_of_service == other.type_of_service;
}

// generates a readable form of the action
std::string of_action_set_ip_type_of_service::to_string() const {
	char buf[16];
	std::string result = "set IP TOS=";
	sprintf(buf, "%d", type_of_service);
	result += buf;

	return result;
}

// generates the action header
uint32_t of_action_set_ip_type_of_service::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_nw_tos), false);
	struct ofp_action_nw_tos* hdr = (struct ofp_action_nw_tos*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) OFPAT_SET_NW_TOS);
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_nw_tos));
	hdr->nw_tos = type_of_service;

	return sizeof(struct ofp_action_nw_tos);
}

// deserializations the set ip TOS action from an autobuf
bool of_action_set_ip_type_of_service::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_nw_tos* hdr = (const struct ofp_action_nw_tos*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != (uint16_t) OFPAT_SET_NW_TOS || hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	type_of_service = hdr->nw_tos;

	return true;
}

// returns the size of the set ip TOS action
uint32_t of_action_set_ip_type_of_service::get_deserialization_size() const {
	return 8;
}

// constructor for action to set TCP/UDP port
of_action_set_tcpudp_port::of_action_set_tcpudp_port() {
	clear();
}

// clears the object
void of_action_set_tcpudp_port::clear() {
	type = action_type::SET_TCPUDP_PORT;
	set_src_port = true;
	port = 0;
}

// checks for equality
bool of_action_set_tcpudp_port::operator==(const of_action_set_tcpudp_port& other) const {
	return set_src_port == other.set_src_port && port == other.port;
}

// generates a readable form of the action
std::string of_action_set_tcpudp_port::to_string() const {
	char buf[16];
	std::string result = std::string(set_src_port ? "set nw_src_port=" : "set nw_dest_port=");
	sprintf(buf, "%hu", port);
	result += buf;

	return result;
}

// generates the action header
uint32_t of_action_set_tcpudp_port::serialize(autobuf& dest) const {

	dest.create_empty_buffer(sizeof(struct ofp_action_tp_port), false);
	struct ofp_action_tp_port* hdr = (struct ofp_action_tp_port*) dest.get_content_ptr_mutable();

	hdr->type = htons((uint16_t) (set_src_port ? OFPAT_SET_TP_SRC : OFPAT_SET_TP_DST));
	hdr->len = htons((uint16_t) sizeof(struct ofp_action_tp_port));
	hdr->tp_port = htons(port);

	return sizeof(struct ofp_action_tp_port);
}

// deserializes the set tcp/udp port action from an autobuf
bool of_action_set_tcpudp_port::deserialize(const autobuf& content) {
	clear();
	const struct ofp_action_tp_port* hdr = (const struct ofp_action_tp_port*) content.get_content_ptr();

	#ifndef __NO_OPENFLOW_SAFETY_CHECKS
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	uint16_t hdr_type = ntohs(hdr->type);
	uint16_t hdr_len = ntohs(hdr->len);

	if (hdr_type != ((uint16_t) OFPAT_SET_TP_SRC && hdr_type != (uint16_t) OFPAT_SET_TP_DST)
		|| hdr_len != get_deserialization_size()) {
		clear();
		return false;
	}
	#endif

	port = ntohs(hdr->tp_port);
	set_src_port = (ntohs(hdr->type) == (uint16_t) OFPAT_SET_TP_SRC);

	return true;
}

// returns the size of the set tcp/udp port action
uint32_t of_action_set_tcpudp_port::get_deserialization_size() const {
	return 8;
}

// constructor for vendor-specific action
of_action_vendor::of_action_vendor() {
	clear();
}

// clears the vendor action
void of_action_vendor::clear() {
	type = action_type::VENDOR;
	vendor = 0;
	request.clear();
}

// checks for equality (not implemented for now -- always returns false)
bool of_action_vendor::operator==(const of_action_vendor& other) const {
	return false;
}

// generates a readable form of the vendor action
std::string of_action_vendor::to_string() const {
	char buf[16];
	std::string result = "vendor opcode=";
	sprintf(buf, "%u", vendor);
	result += buf;
	
	return result;
}

// generates the action header for vendor actions
uint32_t of_action_vendor::serialize(autobuf& dest) const {

	/*
	char null_buf[8];
	autobuf result;
	struct ofp_action_vendor_header header;
	uint16_t original_len = (sizeof(struct ofp_action_vendor_header) + request.size());
	uint16_t padded_len = original_len + original_len % 8;

	memset(&header, 0, sizeof(struct ofp_action_vendor_header));
	memset(null_buf, 0, sizeof(null_buf));
	header.type = htons((uint16_t) OFPAT_VENDOR);
	header.len = htons((uint16_t) padded_len);
	header.vendor = htonl(vendor);

	result.set_content(&header, sizeof(header));
	result += request;
	if (original_len % 8 > 0)
		result.append(null_buf, original_len % 8);

	return result;
	*/

	output::log(output::loglevel::BUG, "of_action_vendor::serialize() error -- not implemented.\n");
	abort();
	return 0;
}

// deserializes the vendor action from an autobuf
bool of_action_vendor::deserialize(const autobuf& content) {
	clear();

	/*
	struct ofp_action_vendor_header hdr;
	if (content.size() < sizeof(hdr)) {
		return false;
	}

	memcpy(&hdr, content.get_content_ptr(), sizeof(hdr));
	hdr.type = ntohs(hdr.type);
	hdr.len = ntohs(hdr.len);

	if (hdr.type != (uint16_t) OFPAT_VENDOR || hdr.len != content.size()) {
		clear();
		return false;
	}

	if (hdr.len > 8) {
		content.read_range(request, 8, content.size()-1);
	}
	
	return true;
	*/

	output::log(output::loglevel::BUG, "of_action_vendor::deserialize() error -- not implemented.\n");
	abort();
	return false;
}

// returns the size of the vendor action
uint32_t of_action_vendor::get_deserialization_size() const {
	return 8 + request.size();
}
