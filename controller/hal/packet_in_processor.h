#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include "../../common/rwqueue.h"

// some forward declarations
class of_message_packet_in;
class raw_packet;
class packet_filter;

// class that handles all packets from the controller
class packet_in_processor {
public:

	// smaller values are earlier in the callback chain
	enum class priority_class : uint32_t {	CAM = 1,
																					ARP = 2,
																					ECHO_SERVER = 3,
																					INTER_IRONSTACK = 4,
																					FLOW_POLICY_CHECKER = 5,
																					ALWAYS_AT_LAST = 6			// sentinel to mark end of priority table
																																	// not a real packet processor!
	};

	// constructor and destructor
	packet_in_processor();
	~packet_in_processor();

	// removes all filters
	void clear();

	// startup and shutdown packet_in processing
	void init(uint32_t num_threads);
	void shutdown();

	// packet filter related
	void enqueue_packet(const shared_ptr<of_message_packet_in>& packet);
	void register_filter(const shared_ptr<packet_filter>& filter);
	void unregister_filter(const shared_ptr<packet_filter>& filter);

private:

	// init and shutdown flags
	atomic<bool> initialized;
	atomic<bool> shutdown_flag;
	atomic<bool> under_initialization;

	// vector of threads that are managed by the packet processor
	vector<thread> packet_processor_threads;

	// pending packet queue
	rwqueue<shared_ptr<of_message_packet_in>> packet_queue;

	// lock for packet filters
	mutex lock;
	vector<pair<shared_ptr<packet_filter>, uint32_t>> packet_filters;

	// packet processing thread entrypoint
	void packet_processing_entrypoint();
};

// packet processing filter
class packet_filter {
public:

	// constructor needs to supply priority class (look at packet_in_processor class for details)
	packet_filter(packet_in_processor::priority_class priority):
		priority_type(priority) {}

	// gets the priority class
	packet_in_processor::priority_class get_packet_in_priority() const { return priority_type; }

	// used to determine if a packet should be entered for processing
	// return true to indicate that the packet has been consumed. returning
	// false will pass the packet on to the next filter.
	virtual bool filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt)=0;

private:

	packet_in_processor::priority_class priority_type;

};

