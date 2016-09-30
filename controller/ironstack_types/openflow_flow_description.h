#pragma once

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <vector>
#include <memory>
#include <inttypes.h>
#include "../openflow_types/of_match.h"
#include "openflow_action_list.h"

using namespace std;
class openflow_flow_description {
public:

	openflow_flow_description();
	openflow_flow_description(const of_match& criteria_, const openflow_action_list& action_list_,
		uint64_t cookie_, uint16_t priority_):criteria(criteria_),
		action_list(action_list_),
		cookie(cookie_),
		priority(priority_) {}

	void   clear();
	string to_string() const;
	string to_string_summarized() const;

	// matches criteria and action list
	// priority and cookies are not checked!
	bool   operator==(const openflow_flow_description& other) const;
	bool   operator!=(const openflow_flow_description& other) const;

	// user-accessible fields
	of_match             criteria;
	openflow_action_list action_list;
	uint64_t             cookie;
	uint16_t             priority;

	// for serialization
	uint32_t serialize(autobuf& dest) const;
	bool     deserialize(const autobuf& content);

};

