#include "openflow_switch_features.h"

// generates a readable version of the object
std::string openflow_switch_features::to_string() const {
	char buf[256];
	std::string result;

	sprintf(buf, "datapath id: %" PRIu64 "\nn_buffers: %u\nn_tables: %d\n",
		datapath_id,
		n_buffers,
		n_tables);

	result = buf;

	result += "switch addr: " + switch_address.to_string() +
		"\n" + capabilities.to_string() + "\n" +
		actions_supported.to_string() + "\n\n";

	return result;
}
