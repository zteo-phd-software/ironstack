#include "ipv6_address.h"
#include <math.h>

// default constructor
ipv6_address::ipv6_address() {
	clear();
}

// copy constructor
ipv6_address::ipv6_address(const ipv6_address& input) {
	set(input);
}

// constructor with shortcut for creating address from string
ipv6_address::ipv6_address(const std::string& input) {
	set(input);
}

ipv6_address::ipv6_address(uint16_t half_word1,
                uint16_t half_word2,
                uint16_t half_word3,
                uint16_t half_word4,
                uint16_t half_word5,
                uint16_t half_word6,
                uint16_t half_word7,
                uint16_t half_word8) {
  set(half_word1, half_word2, half_word3, half_word4, half_word5, half_word6, half_word7, half_word8);
}

// clears the ip address
void ipv6_address::clear() {
	*((uint32_t*) half_words) = 0;
}

// resolves an address TODO
bool ipv6_address::resolve(const std::string& input) {
    return false;
	/*char result[16];
	memset(result, 0, sizeof(result));

	if (cu_resolve_hostname_to_ip_address(input.c_str(), result) == 0) {
		set(std::string(result));
		return true;
	}	else {
		clear();
		return false;
	}*/
}

// sets the address
bool ipv6_address::set(const ipv6_address& input) {
	if (input.half_words == half_words) {
		return true;
	}

  *((uint64_t*) half_words) = *((uint64_t*)input.half_words);
  *((uint64_t*) half_words+1) = *((uint64_t*)input.half_words + 1);
	return true;
}

// sets the address
bool ipv6_address::set(const std::string& input) {
	return extract_address(input);
}

// sets the address
bool ipv6_address::set(uint16_t half_word1,
                uint16_t half_word2,
                uint16_t half_word3,
                uint16_t half_word4,
                uint16_t half_word5,
                uint16_t half_word6,
                uint16_t half_word7,
                uint16_t half_word8) {
  half_words[0] = half_word1;
  half_words[1] = half_word2;
  half_words[2] = half_word3;
  half_words[3] = half_word4;
  half_words[4] = half_word5;
  half_words[5] = half_word6;
  half_words[6] = half_word7;
  half_words[7] = half_word8;
	return true;
}

// sets the address direct from the network buffer
void ipv6_address::set_from_network_buffer(const void* buf) {
  *((uint64_t*) half_words) = *((const uint64_t*) buf);
  *((uint64_t*)half_words + 1) = *((const uint64_t*)buf + 1);
}

// sets the address
ipv6_address& ipv6_address::operator=(const ipv6_address& input) {
	if (input == *this) {
		return *this;
	}

	set(input);
	return *this;
}

// sets the address
ipv6_address& ipv6_address::operator=(const std::string& input) {
	set(input);
	return *this;
}

// compares ip addresses
bool ipv6_address::operator==(const ipv6_address& input) const {
	return *((const uint64_t*) input.half_words) == *((const uint64_t*) half_words)
	&& *((const uint64_t*)input.half_words + 1) == *((const uint64_t*)half_words + 1);
}

// compares ip addresses
bool ipv6_address::operator!=(const ipv6_address& input) const {
	return !(*this == input);
}

// gets the address
std::string ipv6_address::to_string() const {
	char buf[128];
	sprintf(buf, "%u:%u:%u:%u:%u:%u:%u:%u", half_words[0], half_words[1], half_words[2], half_words[3], half_words[4], half_words[5], half_words[6], half_words[7]);
	return std::string(buf);
}

// copies the address in packed form into the given buffer
void ipv6_address::get_address_network_order(void* buf) const {
	memcpy(buf, half_words, sizeof(half_words));
}

// returns true if the address is 0.0.0.0.0.0.0.0
bool ipv6_address::is_nil() const {
	return *((const uint64_t*) half_words) == 0
	&& *((const uint64_t*) half_words + 1) == 0;
}

// returns true if the address is x.x.x.255
bool ipv6_address::is_multicast() const {
  return half_words[0]>>8 == 0xff00;
}

// validates a given ip address. note: a string input with a port specifier behind is also considered valid (even if the port number isn't valid)
bool ipv6_address::validate(const std::string& input) {
	ipv6_address test(input);
	return !test.is_nil();

	return true;
}

// extracts the address given inside a string and copies it into the object
// 0000:0000:0000:0000:0000:0000:0000:0000
bool ipv6_address::extract_address(const std::string& input) {
	int buffer_size = input.size();
	uint16_t values[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int half_word_counter = 0;
	
  // clear the object
	ipv6_address::clear();

  uint16_t hex_to_dec = 0;
  int hex_offset = 3;
	// extract fields
  for (int offset = 0; offset < buffer_size; offset++) {
    if (input[offset] >= '0' && input[offset] <= '9') {
      hex_to_dec += (input[offset] - '0') * pow(16, hex_offset);
      hex_offset--;
    } else if (input[offset] >= 'A' && input[offset] <= 'F') {
      hex_to_dec += (input[offset] - 'A' + 10) * pow(16, hex_offset);
      hex_offset--;
    } else if (input[offset] >= 'a' && input[offset] <= 'f') {
      hex_to_dec += (input[offset] - 'a' + 10) * pow(16, hex_offset);
      hex_offset--;
    }
    if (hex_offset < 0) {
      hex_offset = 3;
      values[half_word_counter] = hex_to_dec;
      hex_to_dec = 0;
      half_word_counter++;
    }
    if (half_word_counter > 8) {
      return false;
    }
  }
	// check blacklist addresses here
	if (values[0] == 0) {
		return false;
	}

	// succeeded
	for (int offset = 0; offset <= 7; offset++) {
		half_words[offset] = (uint16_t) values[offset];
	}

	return true;
}

