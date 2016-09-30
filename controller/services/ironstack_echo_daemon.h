#pragma once
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "../hal/hal.h"
#include "../hal/packet_in_processor.h"
#include "../hal/service_catalog.h"
#include "../hal/hal_transaction.h"
#include "../../common/timer.h"
using namespace std;

// ironstack echo daemon
//
// processes ICMP echo packets and replies/forwards them
// so that the switch appears to be responsive to pings (useful
// for benchmarking switch performance and load).
//
// init this after controller has been initialized.

namespace ironstack {
class echo_daemon : public service, public packet_filter, public enable_shared_from_this<packet_filter> {
public:

	// constructor and destructor
	echo_daemon(service_catalog* ptr):
		service(ptr, service_catalog::service_type::ECHO_SERVER, 1, 0),
		packet_filter(packet_in_processor::priority_class::ECHO_SERVER),
		initialized(false),
		seq(0) {

		dependencies = { service_catalog::service_type::CAM,
			service_catalog::service_type::ARP,
			service_catalog::service_type::SWITCH_STATE
		};

		controller = ptr->get_controller();
		processor = controller->get_packet_processor();
	}
	virtual ~echo_daemon();

	// starts the echo daemon service
	virtual bool init();
	virtual bool init2();
	virtual void shutdown();

	// used to process ping and echo replies
	virtual bool filter_packet(const shared_ptr<of_message_packet_in>& packet, const raw_packet& raw_pkt);

	// pings a host and gets the reply time.
	// automatically does an ARP request if needed.
	// returns the number of milliseconds the response took, or
	// -1 if error timed out.
	int ping(const ip_address& dest, uint16_t vlan_id, int timeout_ms=3000);

	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	// internal class to track echo daemon requests
	class echo_daemon_request {
	public:
		echo_daemon_request(uint32_t seq_in):status(false),seq(seq_in) {}

		timer              request_timer;
		mutex              lock;
		condition_variable cond;
		bool               status;
		uint32_t           seq;
	};

	// service state
	bool                            initialized;
	uint32_t                        seq;
	packet_in_processor*            processor;

	// queue to keep track of outstanding ping requests
	mutable mutex                   lock;
	map<uint32_t, shared_ptr<echo_daemon_request>> wait_queue;

};	// end class def
};	// end namespace ironstack
