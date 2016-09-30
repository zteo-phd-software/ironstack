#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include "arp_table.h"
#include "../../common/dbg.h"
#include "../../common/mac_address.h"
#include "../../common/ip_address.h"
#include "../../common/std_packet.h"
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../hal/packet_in_processor.h"
using namespace std;

class switch_state;

// ARP service
//
// this class manages the ARP table of the switch
// and maps IP to mac addresses. the corresponding class
// that maps mac addresses to ports is now implemented
// in cam.h
//
// capacity management:
// dynamically generated ARP entries are not persistent
// and are subject to capacity rules. static ARP entries
// are always inserted, regardless of capacity.

//
// revision 5 (2/21/15)
// by Z. Teo

class arp : public service, public packet_filter, public enable_shared_from_this<arp> {
public:

	// constructor -- no service dependencies
	arp(service_catalog* ptr):service(ptr, service_catalog::service_type::ARP, 2, 0),
		packet_filter(packet_in_processor::priority_class::ARP),
		initialized(false) {
	
		atomic_store(&requests_forwarded, (uint32_t) 0);
		atomic_store(&requests_serviced, (uint32_t) 0);
		atomic_store(&requests_made, (uint32_t) 0);

		controller = ptr->get_controller();
		if (controller != nullptr) {
			processor = controller->get_packet_processor();
		}
	}

	// destructor
	virtual        ~arp() { shutdown(); };

	// startup and shutdown functions
	virtual bool   init();
	virtual bool   init2();
	virtual void   shutdown();

	// arp table creation utilities
	void           create_arp_table(uint16_t vlan_id);
	void           remove_arp_table(uint16_t vlan_id);
	void           remove_all_arp_tables();
	set<uint16_t>  get_arp_vlans() const;

	// arp table management
	void           clear_arp_table(uint16_t vlan_id);
	void           clear_all_tables();

	// insert an entry into the arp table
	void           insert(const mac_address& dl_address, const ip_address& nw_address, uint16_t vlan_id, bool persistent=false);
	void           insert_all(const mac_address& dl_address, const ip_address& nw_address, bool persistent=false);

	// look up an arp entry
	ip_address     lookup_ip_for(const mac_address& dl_address, uint16_t vlan_id) const;
	mac_address    lookup_mac_for(const ip_address& nw_address, uint16_t vlan_id) const;

	// remove an entry
	void           remove(const mac_address& dl_address, uint16_t vlan_id);
	void           remove(const ip_address& nw_address, uint16_t vlan_id);
	void           remove_all(const mac_address& dl_address);
	void           remove_all(const ip_address& nw_address);

	// retrieve a copy of the arp table for a vlan id
	vector<arp_entry> get_arp_table(uint16_t vlan_id) const;

	// arp query functions
	void           send_arp_query(const ip_address& dest, uint16_t vlan);
	mac_address    send_arp_query_blocking(const ip_address& dest_ip, uint16_t vlan, int timeout_ms=3000);

	// housekeeping information
	uint32_t       get_requests_forwarded() const;
	uint32_t       get_requests_serviced() const;
	uint32_t       get_requests_made() const;

	// packet handling functions
	virtual bool   filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// required by service class
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	// a class for pending queries
	class pending_query_t {
	public:
		pending_query_t():status(false) {}

		ip_address         ip_to_query;
		mac_address        result_mac;

		mutex              lock;
		condition_variable cond;
		bool               status;
	};


	// service status
	bool                             initialized;
	mutable mutex                    table_lock;
	mutable map<uint16_t, arp_table> arp_tables;

	// map of lookups to their timed barrier ids
	mutable mutex                    query_lock;
	map<ip_address, shared_ptr<pending_query_t>> pending_queries;

	// lookup counters
	atomic<uint32_t>                 requests_forwarded;  // number of arp requests seen flowing on the switch
	atomic<uint32_t>                 requests_serviced;   // number of arp requests replied to
	atomic<uint32_t>                 requests_made;       // number of arp requests made by this switch

	// cache of the original packet processor used for ARP registration
	packet_in_processor*  processor;

	// helper function to accelerate flows
	bool accelerate_flow(const mac_address& src_mac, uint16_t vlan_id, uint16_t phy_port);
	bool forward_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);
};

