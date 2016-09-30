#pragma once
#include <mutex>
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../hal/packet_in_processor.h"
using namespace std;

// this class handles unhandled flow packets and installs appropriate rules
// to handle flow rule installation

class flow_policy_checker : public service, public packet_filter, public enable_shared_from_this<flow_policy_checker> {
public:

	// constructor
	flow_policy_checker(service_catalog* ptr):service(ptr, service_catalog::service_type::FLOW_POLICY_CHECKER, 1, 0),
		packet_filter(packet_in_processor::priority_class::FLOW_POLICY_CHECKER),
		initialized(false) {
		controller = ptr->get_controller();
		processor = controller->get_packet_processor();
	}

	// startup and shutdown functions
	virtual bool init();
	virtual bool init2();
	virtual void shutdown();

	// function to accelerate flows (create automatic flows based on src)
	// returns true if installed, false if not installed or disallowed
	bool         accelerate_flow(const mac_address& src, uint16_t vlan_id, uint16_t phy_port);

	// function to foward packets before their flows are accelerated (typically used in callback handlers
	// while pending a flow acceleration)
	bool         forward_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// packet filter function to handle all straggler packets
	virtual bool filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// service-required functions
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	mutex                lock;
	bool                 initialized;
	packet_in_processor* processor;

};
