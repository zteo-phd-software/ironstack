#pragma once

#include <string>
#include "../../common/autobuf_packer.h"
#include "../ironstack_types/openflow_flow_description.h"
#include "../ironstack_types/openflow_flow_description_and_stats.h"
using namespace std;

// this class loads/saves flows, translates flows from ACL to L2
class flow_parser {
public:

	// add flows to the parser
	bool load_flows(const string& filename);
	void load_flows(const vector<openflow_flow_description_and_stats>& flows);
	void load_flows(const vector<openflow_flow_description>& flows);

	// save flows to disk
	bool save_flows(const string& filename) const;

	// performs a translation into L2 rules
	// returns number of flows translated
	uint32_t translate_to_l2(vector<openflow_flow_description>& translated_flows,
		vector<openflow_flow_description>& untranslated_flows) const;
	
private:

	vector<openflow_flow_description> flows;

};
