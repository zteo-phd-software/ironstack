#pragma once

#include <arpa/inet.h>
#include <memory.h>
#include <netdb.h>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
using namespace std;

// updated IPv4 address class (2/19/15)
// revision by Z. Teo

class ip_address {
public:

	// constructors
	ip_address();
	ip_address(const ip_address& input);
	ip_address(const string& input);
	ip_address(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4);

	// assignment operators
	ip_address& operator=(const ip_address& input);
	ip_address& operator=(const string& input);

	// setter and getter methods
	void clear();
	bool resolve(const string& input);	// resolves the string address.
																			// if successful, store into this ip
																			// address object.
	bool set(const ip_address& input);
	bool set(const string& input);
	bool set(uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4);
	void set_from_network_buffer(const void* buf); // supplied in packed network
																								 // endian format
	void set_from_be32(uint32_t val);              // supplied as an uint32_t in
																								 // big endian (network) format
	uint8_t& operator[](int index);
	uint8_t  operator[](int index) const;
	void     get_address_network_order(void* buf) const;
	uint32_t get_as_be32() const;

	// comparators
	bool operator==(const ip_address& input) const;
	bool operator!=(const ip_address& input) const;
	bool operator<(const ip_address& other) const;

	// return ip address as a readable string
	string to_string() const;

	// validation methods
	bool is_nil() const;
	bool is_broadcast() const;

	// ip address validation that does not require object instantiation
	// checks only if the input is a numeric IP
	// if dest is supplied, will store the result into it
	static bool validate_numeric_ip(const string& input, ip_address* dest=nullptr);

	// checks if a given string can be translated into an IP address
	// if the string is a numeric IP address, it is directly converted
	// otherwise the string is checked against the network resolver
	// this is more general than validate_numeric_ip() but may block
	// because of the network name resolution)
	// if dest is specified, will store the result into it
	static bool validate_ip(const string& input, ip_address* dest=nullptr);

private:
	uint8_t octets[4];	// actual storage for the network address
};


// possibly deprecated? used for creating hash values out of an IP address
namespace std {
	template <>
	struct hash<ip_address> : public unary_function<ip_address, size_t> {
		size_t operator()(const ip_address& address) const {
			return (size_t) address.get_as_be32();
		}
	};
};

