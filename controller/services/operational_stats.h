#pragma once

#include <mutex>
#include <string>
#include <vector>
#include "../hal/hal.h"
#include "../hal/service_catalog.h"
#include "../openflow_messages/of_message_stats_reply.h"
using namespace std;

// implements a service to track operational switch statistics
class operational_stats : public service {
public:

	// constructor and destructor
	operational_stats(service_catalog* ptr):
		service(ptr, service_catalog::service_type::OPERATIONAL_STATS, 1, 0),
		initialized(false),
		aggregate_stats_xid(0),
		flow_stats_completed(true),
		flow_stats_xid(0),
		table_stats_completed(true),
		table_stats_xid(0),
		port_stats_completed(true),
		port_stats_xid(0),
		queue_stats_completed(true),
		queue_stats_xid(0) {
		controller = ptr->get_controller(); 
	};
	virtual ~operational_stats() {}

	// startup/shutdown functions
	virtual bool init();
	virtual bool init2();
	virtual void shutdown();

	// update stats
	bool update_all_stats();
	bool update_aggregate_stats();
	bool update_table_stats();
	bool update_flow_stats();
	bool update_port_stats();
	bool update_queue_stats();

	// packet handling to update state
	void update_handler(const shared_ptr<of_message_stats_reply_aggregate_stats>& msg);
	void update_handler(const shared_ptr<of_message_stats_reply_table_stats>& msg);
	void update_handler(const shared_ptr<of_message_stats_reply_flow_stats>& msg);
	void update_handler(const shared_ptr<of_message_stats_reply_port_stats>& msg);
	void update_handler(const shared_ptr<of_message_stats_reply_queue_stats>& msg);

	// retrieve state
	openflow_aggregate_stats                    get_aggregate_stats() const;
	vector<openflow_table_stats>                get_table_stats() const;
	vector<openflow_flow_description_and_stats> get_flow_stats() const;
	vector<openflow_port_stats>                 get_port_stats() const;
	vector<openflow_queue_stats>                get_queue_stats() const;

	// reset state (
	void clear();

	// required from service class
	virtual string get_service_info() const;
	virtual string get_running_info() const;

private:

	mutable mutex lock;
	bool initialized;
	openflow_aggregate_stats aggregate_stats;
	uint32_t aggregate_stats_xid;

	vector<openflow_flow_description_and_stats> flow_stats;
	vector<openflow_flow_description_and_stats> flow_stats_shadow;
	bool flow_stats_completed;
	uint32_t flow_stats_xid;

	vector<openflow_table_stats> table_stats;
	vector<openflow_table_stats> table_stats_shadow;
	bool table_stats_completed;
	uint32_t table_stats_xid;

	vector<openflow_port_stats> port_stats;
	vector<openflow_port_stats> port_stats_shadow;
	vector<openflow_port_stats> port_stats_initial;
	bool port_stats_completed;
	uint32_t port_stats_xid;

	vector<openflow_queue_stats> queue_stats;
	vector<openflow_queue_stats> queue_stats_shadow;
	bool queue_stats_completed;
	uint32_t queue_stats_xid;

};
