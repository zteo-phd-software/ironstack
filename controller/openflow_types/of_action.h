#pragma once

#include <memory.h>
#include <stdio.h>
#include <string>
#include "../../common/mac_address.h"
#include "../../common/autobuf.h"
#include "../../common/ip_address.h"
#include "../../common/openflow.h"
#include "../../common/common_utils_oop.h"

class of_action_output_to_port;
class of_action_enqueue;
class of_action_set_vlan_id;
class of_action_set_vlan_pcp;
class of_action_strip_vlan;
class of_action_set_mac_address;
class of_action_set_ip_address;
class of_action_set_ip_type_of_service;
class of_action_set_tcpudp_port;
class of_action_vendor;

// syntax types for actions
// output_port = [x | controller]
// enqueue_port = [x] queue_id = [x]
// set_vlan = [x]
// set_vlan_pcp = [x]
// strip_vlan = [1 | 0]
// set_dl_src = [x]
// set_dl_dest = [x]
// set_nw_src = [x]
// set_nw_dest = [x]
// set_ip_tos = [x]
// set_src_port = [x]
// set_dest_port = [x]
// vendor <NOT SUPPORTED>

// base class for all flow action types
class of_action {
public:

	// action enumerations (internal)
	enum class action_type { UNKNOWN, OUTPUT_TO_PORT, ENQUEUE, SET_VLAN_ID,
		SET_VLAN_PCP, STRIP_VLAN, SET_DL_ADDR, SET_NW_ADDR, SET_IP_TOS,
		SET_TCPUDP_PORT, VENDOR };

	// base class constructor and destructor
	of_action() { type = action_type::UNKNOWN; }
	virtual ~of_action() {};

	// get the action type
	action_type get_action_type() const { return type; }

	// comparators
	bool operator==(const of_action& other) const;
	bool operator!=(const of_action& other) const { return !(*this == other); }

	// used for checking and cloning the derived class
	static of_action* clone(const of_action* original);
	static action_type infer_type(const autobuf& content);

	virtual void clear()=0;
	virtual std::string to_string() const=0;
	virtual uint32_t serialize(autobuf& dest) const=0;
	virtual bool deserialize(const autobuf& contents)=0;
	virtual uint32_t get_deserialization_size() const=0;

protected:
	action_type type;
};

// outputs to a port
// text syntax: "output_port=[x | controller]"
class of_action_output_to_port : public of_action {
public:
	
	of_action_output_to_port();
	virtual ~of_action_output_to_port() {};

	// comparators
	bool operator==(const of_action_output_to_port& other) const;
	bool operator!=(const of_action_output_to_port& other) const { return !(*this == other); }

	// user-accessible fields
	bool			send_to_controller;		// if set, this ignores the port number (default not set)
	uint16_t	port;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// enqueues packet to a queue
// text syntax: "enqueue_port=x queue_id=x"
class of_action_enqueue : public of_action {
public:

	of_action_enqueue();
	virtual ~of_action_enqueue() {};

	// comparators
	bool operator==(const of_action_enqueue& other) const;
	bool operator!=(const of_action_enqueue& other) const { return !(*this == other); }

	// user-accessible fields
	uint16_t	port;
	uint32_t	queue_id;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets the vlan vid
// text syntax: "set_vlan = x"
class of_action_set_vlan_id : public of_action {
public:

	of_action_set_vlan_id();
	virtual ~of_action_set_vlan_id() {};

	// comparators
	bool operator==(const of_action_set_vlan_id& other) const;
	bool operator!=(const of_action_set_vlan_id& other) const { return !(*this == other); }

	// user-accessible fields
	uint16_t	vlan_id;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets the vlan pcp
// text syntax: "set_vlan_pcp = x"
class of_action_set_vlan_pcp : public of_action {
public:

	of_action_set_vlan_pcp();
	virtual ~of_action_set_vlan_pcp() {};

	// comparators
	bool operator==(const of_action_set_vlan_pcp& other) const;
	bool operator!=(const of_action_set_vlan_pcp& other) const { return !(*this == other); }

	// user-accessible fields
	uint8_t		vlan_pcp;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// strips the vlan header
// text syntax: "strip_vlan = [1 | 0]"
class of_action_strip_vlan : public of_action {
public:

	of_action_strip_vlan();
	virtual ~of_action_strip_vlan() {};

	// comparators (no options for strip vlan, so they are always equal)
	bool operator==(const of_action_strip_vlan& other) const { return true; }
	bool operator!=(const of_action_strip_vlan& other) const { return false; }

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets a new src/dest mac address
// text syntax: "set_dl_src = x " | "set_dl_dest = x" 
class of_action_set_mac_address : public of_action {
public:

	of_action_set_mac_address();
	virtual ~of_action_set_mac_address() {};

	// comparators
	bool operator==(const of_action_set_mac_address& other) const;
	bool operator!=(const of_action_set_mac_address& other) const { return !(*this == other); }

	// user accessible fields
	bool				set_src;	// true means set source, false means set dest
	mac_address new_addr;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets a new src/dest IP address
// text syntax: "set_nw_src = x" | "set_nw_dest = x"
class of_action_set_ip_address : public of_action {
public:

	of_action_set_ip_address();
	virtual ~of_action_set_ip_address() {};

	// comparators
	bool operator==(const of_action_set_ip_address& other) const;
	bool operator!=(const of_action_set_ip_address& other) const { return !(*this == other); }

	// user-accessible fields
	bool				set_src;	// true means set source, false means set dest
	ip_address	new_addr;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets the IP type of service
// text syntax: "set_ip_tos = x"
class of_action_set_ip_type_of_service : public of_action {
public:

	of_action_set_ip_type_of_service();
	virtual ~of_action_set_ip_type_of_service() {};

	// comparators
	bool operator==(const of_action_set_ip_type_of_service& other) const;
	bool operator!=(const of_action_set_ip_type_of_service& other) const { return !(*this == other); }

	// user-accessible fields
	uint8_t type_of_service;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// sets the TCP/UDP transport port
// text syntax: "set_src_port = x" | "set_dest_port = x"
class of_action_set_tcpudp_port : public of_action {
public:

	of_action_set_tcpudp_port();
	virtual ~of_action_set_tcpudp_port() {};

	// comparators
	bool operator==(const of_action_set_tcpudp_port& other) const;
	bool operator!=(const of_action_set_tcpudp_port& other) const { return !(*this == other); }

	// user-accessible fields
	bool			set_src_port;	// true means set source port, false means set dest port
	uint16_t	port;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	virtual uint32_t get_deserialization_size() const;
};

// vendor-specific action
// text syntax: "vendor" NOT SUPPORTED YET
class of_action_vendor : public of_action {
public:
	of_action_vendor();
	virtual ~of_action_vendor() {};

	// comparators
	bool operator==(const of_action_vendor& other) const;
	bool operator!=(const of_action_vendor& other) const { return !(*this == other); }

	// user-accessible fields
	uint32_t	vendor;
	autobuf request;

	virtual void clear();
	virtual std::string to_string() const;
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& contents);
	
	virtual uint32_t get_deserialization_size() const;
};
