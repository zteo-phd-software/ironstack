#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <stdio.h>
#include <string>
#include <stdint.h>
#include "../../common/ip_address.h"
#include "../../common/mac_address.h"
#include "../../common/rwqueue.h"
#include "../../common/tcp.h"
#include "hal_transaction.h"
#include "service_catalog.h"
#include "packet_in_processor.h"

// hardware abstraction layer to isolate users from dealing with the
// OpenFlow switch.

// author: Z. Teo (zteo@cs.cornell.edu)
// revision 5 (2/20/15)

using namespace std;
class hal {
public:

	// constructor
	hal();
	~hal();

	// initializes the openflow controller with a set of services; blocks until a
	// connection is made from the switch. optionally only accept connections
	// from a given set of IP addresses
	bool init(uint16_t port,
		const set<shared_ptr<service>>& services,
		const set<ip_address>& allowable_ip_address=set<ip_address>());

	// stops the openflow controller; releases shared pointers to services.
	void shutdown();

	// sends a ping request to the switch
	bool send_echo_request(int* response_time_ms=nullptr);

	// sends a barrier message to serialize actions; blocks until completion.
	// all pending callbacks will complete after the barrier request
	bool send_barrier_request(int* response_time_ms=nullptr);

	// gets the responsiveness of the switch, as measured by the periodic barrier
	// requests that are issued
	int  get_switch_response_time();

	// sends a raw ethernet packet out (hal will convert the raw bytes into an
	// openflow message).
	bool send_packet(const autobuf& packet, const set<uint16_t>& phy_ports);
	bool send_packet(const autobuf& packet, uint16_t phy_port);

	// send a packet using the controller identity to a given IP or mac address
	// destination. if IP address is unknown, ARP is first sent. if mac address
	// is unknown, the packet is flooded on all spanning tree ports on all vlans.
	// these functions do not block (but messages may be dropped if the ARP
	// times out and the destination cannot be resolved).
	bool send_packet(const autobuf& packet, const ip_address& address);
	bool send_packet(const autobuf& packet, const mac_address& address);

	// gets the service catalog (for registration/unregistration)
	service_catalog* get_service_catalog();

	// shortcut: gets a service from the service catalog
	shared_ptr<service> get_service(service_catalog::service_type svc_type);

	// gets the packet processing unit
	packet_in_processor* get_packet_processor();

	// enqueues a hal transaction
	void enqueue_transaction(const shared_ptr<hal_transaction>& transaction);

private:
	
	// disable copying and cloning
	hal(const hal& other)=delete;
	hal& operator=(const hal& other)=delete;

	// thread entrypoints
	void send_loop_entrypoint();		// sends pending controller messages in the pending queue
	void recv_loop_entrypoint();		// receives controller messages and parses them
	void process_message_loop_entrypoint();
	void maintenance_thread_entrypoint(); // sends out a barrier message every x seconds if
	                                      // the transaction queue is not empty

	// is the controller ready to perform actions?
	atomic<bool>  switch_ready;
	atomic<bool>  under_initialization;
	atomic<bool>  shutdown_flag;

	// TCP connection to the switch
	tcp connection;

	// in and out queues
	rwqueue<shared_ptr<hal_transaction>> pending_transactions;    // waiting to get to the switch
	rwqueue<shared_ptr<of_message>>      asynchronous_messages;   // async switch to controller msgs
                                                                // that need separate handling
	mutex                                callback_transactions_lock;
	list<shared_ptr<hal_transaction>>    callback_transactions;   // waiting for callbacks

	// external handler for packet_in messages
	packet_in_processor packet_processor;

	// service catalog
	service_catalog svc_catalog;

	// thread IDs
	thread send_loop_thread;
	thread recv_loop_thread;
	thread process_loop_thread;
	thread maintenance_thread;

	// switch responsiveness
	atomic_int switch_response_time_ms;
};

