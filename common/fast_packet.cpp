#include "fast_packet.h"

void raw_packet::clear() {
	raw_packet_buf.reset();
}

string raw_packet::to_string() {
	return string();
}

uint16_t in_port() {
	return in_port;
}

packet_t 
