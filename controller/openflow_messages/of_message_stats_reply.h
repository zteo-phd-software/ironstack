#pragma once

// file: of_message_stats_reply.h
// author: Z. Teo (zteo@cs.cornell.edu)
//
// classes: of_message_stats_reply_switch_description
//			of_message_stats_reply_flow_stats
//			of_message_stats_reply_aggregate_stats
//			of_message_stats_reply_table_stats
//			of_message_stats_reply_port_stats
//			of_message_stats_reply_queue_stats

#include "of_message.h"
#include "../openflow_types/of_action.h"
#include "../ironstack_types/openflow_switch_description.h"
#include "../ironstack_types/openflow_flow_description_and_stats.h"
#include "../ironstack_types/openflow_aggregate_stats.h"
#include "../ironstack_types/openflow_table_stats.h"
#include "../ironstack_types/openflow_queue_stats.h"
#include "../ironstack_types/openflow_port_stats.h"

using namespace std;

// abstract class for stats reply
class of_message_stats_reply : public of_message {
public:

	enum class stats_t { SWITCH_DESCRIPTION, FLOW_STATS, AGGREGATE_STATS, TABLE_STATS, PORT_STATS, QUEUE_STATS };

	of_message_stats_reply() {};
	virtual ~of_message_stats_reply() {};

	stats_t stats_type;
	
	virtual uint32_t serialize(autobuf& dest) const=0;
	virtual bool deserialize(const autobuf& input)=0;
};


// reply to switch description request
class of_message_stats_reply_switch_description : public of_message_stats_reply {
public:
	of_message_stats_reply_switch_description();
	virtual ~of_message_stats_reply_switch_description() {};
	
	virtual void clear();
	string to_string() const;

	// user-accessible fields
	bool		more_to_follow;
	openflow_switch_description description;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// reply to request for individual flow statistics
class of_message_stats_reply_flow_stats : public of_message_stats_reply {
public:

	of_message_stats_reply_flow_stats();
	virtual ~of_message_stats_reply_flow_stats() {};

	virtual void clear();
	string to_string() const;

	// user-accessible fields
	bool					more_to_follow;
	vector<openflow_flow_description_and_stats> flow_stats;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// reply to request for aggregate flow statistics
class of_message_stats_reply_aggregate_stats : public of_message_stats_reply {
public:
	of_message_stats_reply_aggregate_stats();
	virtual ~of_message_stats_reply_aggregate_stats() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	bool		more_to_follow;
	openflow_aggregate_stats aggregate_stats;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// reply to request for flow table statistics
class of_message_stats_reply_table_stats : public of_message_stats_reply {
public:
	of_message_stats_reply_table_stats();
	virtual ~of_message_stats_reply_table_stats() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	bool		more_to_follow;
	vector<openflow_table_stats> table_stats;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// reply to request for physical port stats
class of_message_stats_reply_port_stats : public of_message_stats_reply {
public:
	of_message_stats_reply_port_stats();
	virtual ~of_message_stats_reply_port_stats() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	bool      more_to_follow;
	vector<openflow_port_stats> port_stats;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// reply to request for queue stats
class of_message_stats_reply_queue_stats : public of_message_stats_reply {
public:
	of_message_stats_reply_queue_stats();
	virtual ~of_message_stats_reply_queue_stats() {};

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	bool		more_to_follow;
	vector<openflow_queue_stats> queue_stats;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
