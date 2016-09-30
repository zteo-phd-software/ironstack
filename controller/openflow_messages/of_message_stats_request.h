#pragma once

#include "../openflow_types/of_match.h"
#include "of_message.h"
using namespace std;

// requests description information about the switch
class of_message_stats_request_switch_description : public of_message {
public:
	of_message_stats_request_switch_description();
	
	virtual void clear();
	virtual string to_string() const;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// requests flow statistics
class of_message_stats_request_flow_stats : public of_message {
public:
	of_message_stats_request_flow_stats();

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	of_match	fields_to_match;

	bool		all_tables;			// if turned on, matches for every table is retrieved
	uint8_t		table_id;			// if all_tables is turned off, only this table id is considered

	bool		restrict_out_port;	// if turned off, no restriction on entries matching output ports
	uint16_t	out_port;			// requires entries to match this as an output port

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);

};

// requests aggregate flow statistics
class of_message_stats_request_aggregate_stats : public of_message {
public:
	of_message_stats_request_aggregate_stats();

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	of_match	fields_to_match;

	bool     all_tables;    // if turned on, matches for every table is retrieved
	uint8_t  table_id;      // if all_tables is turned off, only this table id is considered

	bool     restrict_out_port; // if turned off, no restrictions on multiple flows matching this output port
	uint16_t out_port;          // requires matching aggregate flows to match this as an output port

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// requests flow table statistics
class of_message_stats_request_table_stats : public of_message {
public:
	of_message_stats_request_table_stats();

	virtual void clear();
	virtual string to_string() const;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// requests physical port statistics
class of_message_stats_request_port_stats : public of_message {
public:
	of_message_stats_request_port_stats();

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	bool      all_ports;    // turn this on to request information on all ports
	uint16_t  port;         // if all_ports is turned off, then only stats on this port will be returned

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// requests queue statistics
class of_message_stats_request_queue_stats : public of_message {
public:
	of_message_stats_request_queue_stats();

	virtual void clear();
	virtual string to_string() const;

	// user-accessible data
	bool      all_ports;
	uint16_t  port;

	bool      all_queues;
	uint32_t  queue_id;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};

// vendor-specific extension
class of_message_stats_request_vendor_stats : public of_message {
public:
	of_message_stats_request_vendor_stats();

	virtual void clear();
	virtual string to_string() const;

	// user-accessible fields
	uint32_t	vendor_id;
	autobuf	request;

	// serialization functions
	virtual uint32_t serialize(autobuf& dest) const;
	virtual bool deserialize(const autobuf& input);
};
