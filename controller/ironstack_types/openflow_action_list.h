#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../openflow_types/of_action.h"
#include "../../common/autobuf.h"
#include "../../common/common_utils_oop.h"
using namespace std;

class openflow_action_list {
public:
	openflow_action_list();
	openflow_action_list(const openflow_action_list& original);
	openflow_action_list& operator=(const openflow_action_list& original);
	~openflow_action_list();

	// comparators
	bool   operator==(const openflow_action_list& other) const;
	bool   operator!=(const openflow_action_list& other) const;

	void   clear();
	string to_string() const;

	bool   from_string(const string& input);
	void   add_action(const of_action& action);
	void   remove_action(const of_action& action);
	vector<unique_ptr<of_action>> get_actions() const;

	uint32_t serialize(autobuf& dest) const;
	uint32_t get_serialization_size() const;
	bool     deserialize(const autobuf& input);

private:

	vector<unique_ptr<of_action>> actions;

};

