#ifndef __OPENFLOW_QUEUE_CONFIGURATION
#define __OPENFLOW_QUEUE_CONFIGURATION

#include <assert.h>
#include <string>
#include <arpa/inet.h>
#include <stdio.h>
#include "../../common/autobuf.h"
#include "../../common/openflow.h"

// queue configuration (for bandwidth provisioning) -- not really used
// author: Z. Teo (zteo@cs.cornell.edu)
// TODO  : check code (length confusion in implementation)

class of_queue_config {
public:

	of_queue_config();
	of_queue_config(const autobuf& input);

	void clear();

	// user-accessible fields
	uint32_t	queue_id;
	bool			has_property;		// indicates if additional properties are available

	bool			min_rate_enabled;	// indicates if minimum rate has been enabled or disabled
	uint16_t	min_rate;			// minimum rate (in 1/10 of a percent increments)

	bool deserialize(const autobuf& input, uint16_t* bytes_used);
	static uint16_t count_queues(const autobuf& input);

	void set(const autobuf& input);
	std::string to_string() const;
};


#endif
