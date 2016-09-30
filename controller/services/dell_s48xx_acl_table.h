#pragma once
#include <string>
#include <vector>
#include "flow_table.h"
using namespace std;

// dell s48xx ACL table (accepts most flow rules)
class dell_s48xx_acl_table : public flow_table {
public:

	dell_s48xx_acl_table() { max_capacity = 100; }

	// the acl table admits most rules. for the time being,
	// admit all rules but note that flow installation can
	// still fail.
	virtual bool check_table_fit(const of_match& criteria,
		const openflow_action_list& action_list) const;

	// returns the name of the table
	virtual string get_table_name() const;
};
