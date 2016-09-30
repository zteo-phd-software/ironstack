#pragma once

#include <condition_variable>
#include <mutex>
#include <memory>
#include "switch_db.h"
#include "aux_switch_info.h"
#include "../../common/dbg.h"
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../ironstack_types/openflow_vlan_port.h"
#include "../ironstack_types/openflow_switch_description.h"
#include "../openflow_messages/of_message_features_reply.h"
#include "../openflow_messages/of_message_get_config_reply.h"
#include "../openflow_messages/of_message_port_status.h"
#include "../openflow_messages/of_message_stats_reply.h"
using namespace std;

// subclass this to receive callbacks whenever an openflow port changes in
// status
class switch_port_modification_callbacks {
public:
	virtual void switch_port_modification_callback(const openflow_vlan_port& port)=0;
};

// switch state class
// provides management of ports (changing up/down status) callbacks for port
// status changes central location for switch information (ip address, mac
// address, desc, features)
//
// init depends on the CAM and ARP services being already active
class switch_state : public service {
public:

	// constructor	
	switch_state(service_catalog* ptr):
		service(ptr, service_catalog::service_type::SWITCH_STATE, 2, 0),
		initialized(false),
		mac_override_set(false),
		mac_override(false),
		switch_config_set(false),
		switch_description_set(false),
		switch_features_and_ports_set(false) { 

		dependencies = { service_catalog::service_type::CAM, service_catalog::service_type::ARP };

		controller = ptr->get_controller(); 
	};

	// populates auxiliary switch information by supplying a controller IP address
	// looks up the switch db and internally calls set_aux_switch_info().
	bool                        setup_aux_info(const ip_address& addr);

	// required by service
	virtual bool                init();
	virtual bool                init2();
	virtual void                shutdown();

	// wait functions to support switch
	void                        wait_until_switch_ready();
	bool                        is_switch_ready() const;

	// functions for hal to set up the initial switch state
	void                        set_switch_description(const of_message_stats_reply_switch_description& desc);
	void                        set_switch_features(const of_message_features_reply& features);
	void                        set_switch_config(const of_message_get_config_reply& config);

	// functions for hal to update port state
	void                        update_switch_port(const of_message_port_status& status);

	// setup switch metadata
	void                        set_switch_ip(const ip_address& address);
	void                        set_switch_mac_override(bool enabled, const mac_address& mac);
	void                        set_name(const string& name);
	void                        set_aux_switch_info(const aux_switch_info& info);

	// functions to get switch state information
	openflow_switch_description get_switch_description() const;
	openflow_switch_features    get_switch_features() const;
	openflow_switch_config      get_switch_config() const;

	// read switch metadata
	ip_address                  get_switch_ip() const;
	mac_address                 get_switch_mac() const;
	string                      get_name() const;
	aux_switch_info             get_aux_switch_info() const;

	// vlan functions
	void                        set_vlan_ports(const set<uint16_t>& ports, uint16_t vlan_id);
	void                        set_vlan_port_tagging(uint16_t port, bool state);
	void                        set_vlan_port_tagging(const set<uint16_t> ports, bool state);
	void                        set_vlan_flood_ports(const set<uint16_t>& ports, uint16_t vlan_id);

	// gets the set of flood/stp/tagged/untagged ports for a given vlan
	set<uint16_t>               get_flood_ports(uint16_t vlan_id) const;
	set<uint16_t>               get_stp_ports(uint16_t vlan_id) const;
	set<uint16_t>               get_untagged_ports(uint16_t vlan_id) const;
	set<uint16_t>               get_tagged_ports(uint16_t vlan_id) const;

	// get vlans and ports
	set<uint16_t>               get_vlan_ids() const;
	vector<openflow_vlan_port>  get_vlan_ports(uint16_t vlan) const;
	vector<openflow_vlan_port>  get_all_switch_ports() const;
	bool                        get_switch_port(uint16_t port, openflow_vlan_port& result);

	// activate or deactivate port
	void                        set_port_enabled(uint16_t port, bool status);
	bool                        is_port_enabled(uint16_t port) const;

	// callback functionality. callbacks are triggered to the object whenever ports associated with a vlan change
	void                        register_port_modification_callback(const shared_ptr<switch_port_modification_callbacks>& callback, uint16_t vlan_id);
	void                        unregister_port_modification_callback(const shared_ptr<switch_port_modification_callbacks>& callback, uint16_t vlan_id);

	// get information about switch database
	switch_db                   get_db() const;

	// required by service class
	virtual string              get_service_info() const;
	virtual string              get_running_info() const;

private:

	// generic lock for accessing initialized flag and all switch data
	mutable mutex               lock;
	bool                        initialized;

	mutable mutex               switch_waiters_lock;
	mutable condition_variable  switch_waiters_cond;

	bool mac_override_set;         // allowed to set overrides exactly once
	bool mac_override;             // overrides the mac settings as indicated in
	                               // the switch description (useful for
	                               // partitioning switches into multiple
	                               // instances)

	mac_address switch_mac;        // overriden switch mac address
	ip_address  switch_ip_address; // IP address of switch
	string      switch_name;       // switch name
	aux_switch_info aux_info;      // aux info from the csv parser

	switch_db db;                  // database of all switch info

	// switch-specific information
	bool                        switch_config_set;
	openflow_switch_config      switch_config;  // sw fragmentation type and max
	                                            // packet_in len
	bool                        switch_description_set;
	openflow_switch_description switch_description; // sw serial number,
	                                                // manufacturer info, etc
	bool                        switch_features_and_ports_set;
	openflow_switch_features	  switch_features;    // sw mac address, capabilities
	                                                // and actions supported
	bool                        vlans_set;

	// switch port-specific information
	// maps from port number to object
	map<uint16_t, openflow_vlan_port>  ports;

	// callbacks and their locks
	mutex callback_lock;
	vector<pair<uint16_t, weak_ptr<switch_port_modification_callbacks>>> port_mod_callbacks;

	// create placeholder port if one doesn't already exist
	void maybe_create_openflow_vlan_port_unsafe(uint16_t port);
	void create_or_update_openflow_vlan_port_unsafe(const openflow_port& port);
	void remove_openflow_vlan_port_unsafe(uint16_t port);

	void perform_port_mod_callbacks(const openflow_vlan_port& port);
	void check_init_state_unsafe();

	// other misc functions
	bool setup_switch_vlans(const string& filename);	// TODO -- this is deprecated now
};

