#pragma once

#include "autobuf.h"
#include "ip_address.h"
#include <string>
#include <stdint.h>
using namespace std;

// updated IPv4 address & port class (2/19/15)
// revision by Z. Teo

class ip_port {
public:

	ip_port();
	ip_port(const string& input);
	ip_port(const string& address, uint16_t port);
	ip_port(const ip_address& address, uint16_t port);
	ip_port(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4, uint16_t port);

	// setter and getter methods
	void        clear();
	bool        set(const ip_address& addr, uint16_t port);
	bool        set(const string& addr, uint16_t port);
	bool        set(const string& addr_and_port);
	bool        set(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4, uint16_t port);

	bool        set_address(const ip_address& address);
	bool        set_address(const string& address);
	bool        set_address(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4);
	bool        set_port(uint16_t port);
	uint16_t    get_port() const;
	ip_address  get_address() const;

	// comparators
	bool        operator==(const ip_port& input) const;
	bool        operator!=(const ip_port& input) const;

	// validation methods
	bool        is_nil() const;

	// serialization methods
	autobuf     serialize() const;
	bool        deserialize(const autobuf& input);

	// generates a readable version of the object
	string      to_string() const;

	// validation tools
	static bool validate_ip_port(const string& input, ip_port* dest=nullptr);

private:

	// fields
	ip_address address;
	uint16_t   port;

	// support structures
	typedef struct ip_port_hdr {
		uint8_t address[4];
		uint8_t port[2];
	} ip_port_hdr_t;

	bool extract_ip_port(const string& input);

};
