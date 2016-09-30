#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include "cam_table.h"
#include "../../common/mac_address.h"
#include "../../common/timed_barrier.h"
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../hal/packet_in_processor.h"
using namespace std;

// implements the CAM functionality in a switch
//
// this class needs to be on the fast path since it will get called often
// for alternate ports to destination, use the multipath class
//
// revision 2 (2/22/15)
// by Z. Teo

class cam : public service, public packet_filter, public enable_shared_from_this<packet_filter> {
public:

	// constructor
	cam(service_catalog* ptr):service(ptr, service_catalog::service_type::CAM, 2, 0),
		packet_filter(packet_in_processor::priority_class::CAM),
		initialized(false) { 
		controller = ptr->get_controller();
		processor = controller->get_packet_processor();
	};
	virtual ~cam() { shutdown(); }

	// startup and shutdown functions
	virtual bool  init();
	virtual bool  init2();
	virtual void  shutdown();

	// cam table creation
	void          create_cam_table(uint16_t vlan_id);
	void          remove_cam_table(uint16_t vlan_id);
	void          remove_all_cam_tables();
	set<uint16_t> get_cam_vlans() const;
	
	// cam table management
	void          clear_cam_table(uint16_t vlan_id);
	void          clear_all_tables();
	
	// insert an entry into the cam table
	void          insert(const mac_address& dl_addr, uint16_t vlan, uint16_t phy_port);
	void          insert_all(const mac_address& dl_address, uint16_t phy_port);

	// looks up a cam entry (returns -1 if not found)
	int           lookup_port_for(const mac_address& dl_addr, uint16_t vlan) const;

	// delete an entry
	void          remove(const mac_address& dl_addr, uint16_t vlan);
	
	// delete all entries from a port across all vlans
	void          remove(uint16_t phy_port);

	// retrieve a copy of the cam table for a vlan
	map<mac_address, cam_entry> get_cam_table(uint16_t vlan) const;

	// packet handling function (to sniff all packets and vlans)
	virtual bool filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// required from service class
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	bool                            initialized;
	mutable mutex                   lock;
	map<uint16_t, cam_table>        cam_tables;
	packet_in_processor*            processor;
};

