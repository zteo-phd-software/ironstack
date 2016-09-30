#include "openflow_flow_rate.h"

// constructor
openflow_flow_rate::openflow_flow_rate() {
	memset(byte_history, 0, sizeof(byte_history));
	memset(packet_history, 0, sizeof(packet_history));
	for (auto& t : timer_history) {
		t.reset();
	}
	initial_bytes = 0;
	initial_packets = 0;
	byte_offset = 0;
	packet_offset = 0;
	valid_histories = 0;
}

// update flow counters
void openflow_flow_rate::update_flow(const openflow_flow_description_and_stats& new_flow) {
	
	flow = new_flow;
	int num_elements = sizeof(byte_history)/sizeof(uint64_t);

	if (valid_histories < num_elements) {
		
		if (valid_histories == 0) {
			initial_bytes = flow.byte_count;
			initial_packets = flow.packet_count;
			byte_history[0] = 0;
			packet_history[0] = 0;
		} else {

			byte_history[valid_histories] = flow.byte_count-initial_bytes-byte_history[valid_histories-1];
			packet_history[valid_histories] = flow.packet_count-initial_packets-packet_history[valid_histories-1];
		}
		++valid_histories;

	} else {

		for (int counter = 1; counter < num_elements; ++counter) {
			byte_history[counter-1] = byte_history[counter];
			packet_history[counter-1] = packet_history[counter];
		}

		byte_history[num_elements-1] = flow.byte_count-initial_bytes-byte_history[num_elements-1];
		packet_history[num_elements-1] = flow.packet_count-initial_packets-packet_history[num_elements-1];

	}
}

// resets the byte counter; doesn't change rate counter
void openflow_flow_rate::reset_byte_counter() {
	byte_offset = flow.byte_count;
}

// resets the packet counter; doesn't change rate counter
void openflow_flow_rate::reset_packet_counter() {
	packet_offset = flow.packet_count;
}

// read the byte rate counter
uint64_t openflow_flow_rate::get_byte_rate() const {

	if (valid_histories <= 1) return 0;

	// compute total bytes
	uint64_t total_bytes = 0;
	for (int counter = 0; counter < valid_histories; ++counter) {
		total_bytes += byte_history[counter];
	}

	// compute total time
	int time_elapsed_s = (timer_history[0].get_time_elapsed_ms() - timer_history[valid_histories-1].get_time_elapsed_ms())/1000;
	if (time_elapsed_s == 0) return 0;
	return total_bytes/time_elapsed_s;
}

// read the packet rate counter
uint64_t openflow_flow_rate::get_packet_rate() const {

	if (valid_histories <= 1) return 0;

	// compute total packets
	uint64_t total_packets = 0;
	for (int counter = 0; counter < valid_histories; ++counter) {
		total_packets += packet_history[counter];
	}

	// compute total time
	int time_elapsed_s = (timer_history[0].get_time_elapsed_ms() - timer_history[valid_histories-1].get_time_elapsed_ms())/1000;

	if (time_elapsed_s == 0) return 0;
	return total_packets/time_elapsed_s;
}

// read the byte counter
uint64_t openflow_flow_rate::get_byte_count() const {
	return flow.byte_count - byte_offset;
}

// read the packet counter
uint64_t openflow_flow_rate::get_packet_count() const {
	return flow.packet_count - packet_offset;
}

// returns the flow
openflow_flow_description_and_stats openflow_flow_rate::get_flow() const {
	return flow;
}
