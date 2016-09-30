#include "inter_ironstack_service.h"
#include "../gui/output.h"

// initializes the inter-ironstack communications service
bool inter_ironstack_service::init() {
	lock_guard<mutex> g(lock);
	if (initialized) return true;

	processor = controller->get_packet_processor();
	processor->register_filter(shared_from_this());
	initialized = true;
	return true;
}

// called after the controller is online
bool inter_ironstack_service::init2() {
	return true;
}

// stops the inter-ironstack packet processing service
void inter_ironstack_service::shutdown() {
	lock_guard<mutex> g(lock);
	initialized = false;
}

// broadcasts a message to all neighbors informing them of the need to invalidate l2 flow entries
void inter_ironstack_service::announce_invalidate_mac(const list<mac_address>& addresses_to_invalidate) {

}

// handles packet processing
bool inter_ironstack_service::filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt) {

	

	// return true if we ate the packet
	return true;
}

// returns service information
string inter_ironstack_service::get_service_info() const {
	char buf[128];
	sprintf(buf, "inter-ironstack service version %d.%d", service_major_version, service_minor_version);
	return string(buf);
}

// returns running information about the service
string inter_ironstack_service::get_running_info() const {
	return string();
}
