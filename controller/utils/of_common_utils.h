#ifndef __OPENFLOW_COMMON_UTILITIES
#define __OPENFLOW_COMMON_UTILITIES

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
#include "../../common/tcp.h"
using namespace std;

namespace of_common
{
	// translation utilities
	string uint32_to_binary(uint32_t input);
	string uint8_array_to_binary(uint8_t* input, uint32_t len);
	string uint8_to_binary(uint8_t input);

	string binary_to_hex_output(const autobuf& input);
	string port_to_string(uint16_t port);
	string msg_type_to_string(enum ofp_type type);

	// output utilities
	void pad_to_position(std::string& input, uint32_t position);

};

#endif
