#include "packet_in_processor.h"
#include "../openflow_messages/of_message_packet_in.h"
#include "../../common/std_packet.h"
#include "../gui/output.h"

// constructor
packet_in_processor::packet_in_processor() {
	atomic_store(&initialized, false);
	atomic_store(&shutdown_flag, false);
	atomic_store(&under_initialization, false);
}

// destructor
packet_in_processor::~packet_in_processor() {
	shutdown();
}

// starts the packet in processor service
void packet_in_processor::init(uint32_t num_threads) {

	bool expected = false;
	if (!under_initialization.compare_exchange_strong(expected, true)) {
		output::log(output::loglevel::WARNING, "packet_in_processor::init() -- already under initialization.\n");
		return;
	}

	// note that initialized and shutdown_flag cannot both be true since the shutdown
	// function toggles the initialized flag first
	if (atomic_load(&initialized) && !atomic_load(&shutdown_flag)) {
		output::log(output::loglevel::INFO, "packet_in_processor::init() -- already initialized.\n");
		return;
	}

	// setup state before threads are fired up
	atomic_store(&shutdown_flag, false);

	// create packet processor threads
	packet_processor_threads.reserve(num_threads);
	for (uint32_t counter = 0; counter < num_threads; ++counter) {
		packet_processor_threads.push_back(move(thread(&packet_in_processor::packet_processing_entrypoint, this)));
	}

	atomic_store(&initialized, true);

	output::log(output::loglevel::INFO, "packet_in_processor::init() %u thread(s) started.\n", num_threads);
}

// shuts down the packet processor threads
void packet_in_processor::shutdown() {

	// sanity checks
	bool expected = true;
	if (!initialized.compare_exchange_strong(expected, false)) {
		output::log(output::loglevel::WARNING, "packet_in_processor::shutdown() -- packet in processor not initialized.\n");
		return;
	}

	expected = false;
	if (!shutdown_flag.compare_exchange_strong(expected, true)) {
		output::log(output::loglevel::WARNING, "packet_in_processor::shutdown() -- shutdown() already called.\n");
		return;
	}

	output::log(output::loglevel::INFO, "packet_in_processor now shutting down.\n");

	// store some nullptrs so each thread will quit
	uint32_t num_processors = packet_processor_threads.size();
	for (uint32_t counter = 0; counter < num_processors; ++counter) {
		packet_queue.enqueue(nullptr);
	}

	// join all the threads
	for (auto& it : packet_processor_threads) {
		it.join();
	}

	atomic_store(&shutdown_flag, false);
	output::log(output::loglevel::INFO, "packet_in_processor shutdown complete.\n");
}

// removes all filters from the packet processor
void packet_in_processor::clear() {
	lock_guard<mutex> g(lock);
	packet_filters.clear();
}

// stores a packet in event for processing (storing nullptr will quit the thread)
void packet_in_processor::enqueue_packet(const shared_ptr<of_message_packet_in>& packet) {
	packet_queue.enqueue(packet);
}

// registers a filter for packet processing
void packet_in_processor::register_filter(const shared_ptr<packet_filter>& filter) {

	uint32_t priority = (uint32_t) filter->get_packet_in_priority();
	lock_guard<mutex> g(lock);

	auto iterator = packet_filters.begin();
	for (; (iterator != packet_filters.end()) && (iterator->second <= priority); ++iterator);
	packet_filters.insert(iterator, make_pair(filter, priority));
}

// unregisters a filter from packet processing
void packet_in_processor::unregister_filter(const shared_ptr<packet_filter>& filter) {

	lock_guard<mutex> g(lock);
	auto iterator = packet_filters.begin();
	while (iterator != packet_filters.end()) {
		if (iterator->first == filter) {
			packet_filters.erase(iterator);
			break;
		}
		++iterator;
	}
}

// packet processing thread entrypoint
void packet_in_processor::packet_processing_entrypoint() {

	shared_ptr<of_message_packet_in> current_packet;
	vector<pair<shared_ptr<packet_filter>, uint32_t>> packet_filter_copy;
	raw_packet raw_pkt;
	bool handled;

	while (!atomic_load(&shutdown_flag)) {
		
		current_packet = packet_queue.dequeue();
		if (current_packet == nullptr) continue;
		if (!raw_pkt.deserialize(current_packet->pkt_data)) {
			output::log(output::loglevel::WARNING, "packet_in_processor::packet_processing_entrypoint() "
				"-- failed to deserialize raw packet. this packet originated from port %hu.\n", current_packet->in_port);
		}

		// make a copy of the most current set of packet filters
		{
			lock_guard<mutex> g(lock);
			packet_filter_copy = packet_filters;
		}

		handled = false;
		for (const auto filter : packet_filter_copy) {
			if (filter.first->filter_packet(current_packet, raw_pkt)) {
				handled = true;
				break;
			}
		}

		/*
		if (!handled) {
			output::log(output::loglevel::WARNING, "packet_in_processor: packet not handled.\n");
		}
		*/
	}
}

