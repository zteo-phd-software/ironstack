#pragma once

#include <set>
#include <string>
#include "openflow_port.h"
using namespace std;

// used by switch state for openflow vlans
class openflow_vlan_port {
public:

	// constructors
	openflow_vlan_port();

	// resets the vlan port
	void           clear();

	// setter functions
	// note: adding to flood/stp automatically implies membership in vlan
	//       likewise, removing membership from vlan automatically imples
	//       removal from flood/stp membership.
	//
	// also note that untagged vlan ports can only belong to one vlan id.
	// adding more vlan IDs to an untagged port will result in an abort()
	// error.
	void           set_valid(bool state);
	void           set_vlan_tagging(bool state);
	void           add_as_flood_port_for_vlan(uint16_t vlan_id);
	void           add_as_stp_port_for_vlan(uint16_t vlan_id);
	void           add_to_vlan_membership(uint16_t vlan_id);
	void           remove_as_flood_port_for_vlan(uint16_t vlan_id);
	void           remove_as_stp_port_for_vlan(uint16_t vlan_id);
	void           remove_from_vlan_membership(uint16_t vlan_id);
	void           remove_from_all_vlans();
	void           set_openflow_port(const openflow_port& port);

	// getter functions
	bool           is_valid() const;
	bool           is_tagged_port() const;
	bool           is_flood_port_for_vlan(uint16_t vlan_id) const;
	bool           is_stp_port_for_vlan(uint16_t vlan_id) const;
	bool           is_member_of_vlan(uint16_t vlan_id) const;
	set<uint16_t>  get_all_vlans() const;
	openflow_port& get_openflow_port();
	const openflow_port& get_openflow_port_const() const;

	// generates a readable version of this object
	string         to_string() const;

private:

	bool           valid;
	bool           tagged;
	set<uint16_t>  vlan_flood_port_membership;
	set<uint16_t>  vlan_stp_membership;
	set<uint16_t>  vlan_membership;
	openflow_port  port;
};
