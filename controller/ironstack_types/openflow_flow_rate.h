#pragma once
#include "../../common/timer.h"
#include "openflow_flow_description_and_stats.h"

// computes rate for a single flow
class openflow_flow_rate {
public:

	// constructor
	openflow_flow_rate();

	// called to update the flow internally
	void     update_flow(const openflow_flow_description_and_stats& flow);
	
	// these counters reset bytes and packets, but not the rate
	void     reset_byte_counter();
	void     reset_packet_counter();

	// used to read the counters. rate is units per second.
	uint64_t get_byte_rate() const;
	uint64_t get_packet_rate() const;
	uint64_t get_byte_count() const;
	uint64_t get_packet_count() const;
	openflow_flow_description_and_stats get_flow() const;

private:

	openflow_flow_description_and_stats flow;
	uint64_t byte_history[10];
	uint64_t packet_history[10];
	timer    timer_history[10];
	int      valid_histories;

	uint64_t initial_bytes;
	uint64_t initial_packets;

	uint64_t byte_offset;
	uint64_t packet_offset;

};
