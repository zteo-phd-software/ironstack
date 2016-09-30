#pragma once
#include <mutex>
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../hal/packet_in_processor.h"
using namespace std;

class inter_ironstack_service : public service, public packet_filter, public enable_shared_from_this<inter_ironstack_service> {
public:
	// constructor
	inter_ironstack_service(service_catalog* ptr):service(ptr, service_catalog::service_type::INTER_IRONSTACK, 1, 0),
		packet_filter(packet_in_processor::priority_class::INTER_IRONSTACK),
		initialized(false) {
		controller = ptr->get_controller();
		processor = controller->get_packet_processor();
	}

	// destructor
	virtual ~inter_ironstack_service() { shutdown(); }

	// startup and shutdown functions
	virtual bool init();
	virtual bool init2();
	virtual void shutdown();

	// used primarily by the flow service to inform all other controllers to invalidate L2 flow entries
	void    announce_invalidate_mac(const list<mac_address>& addresses_to_invalidate);


	// used for processing inegress packets
	virtual bool filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// required by service class
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	mutex                lock;
	bool                 initialized;
	packet_in_processor* processor;
};
