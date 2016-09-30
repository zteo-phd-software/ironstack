#pragma once
#include <vector>
#include "flow_table.h"
using namespace std;

// dell s48xx L2 table
class dell_s48xx_l2_table : public flow_table {
public:

	dell_s48xx_l2_table() { max_capacity = 48000; }

	// for dell s48xx l2 table, the criteria must be:
	// dl_vlan = xx and dl_dst = xx. all other fields wildcarded
	// the action list must be:
	// output_port = xx ONLY
	virtual bool check_table_fit(const of_match& criteria,
		const openflow_action_list& action_list) const;

	// returns the name of the table
	virtual string get_table_name() const;
};
