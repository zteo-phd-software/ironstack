#ifndef __OPENFLOW_SWITCH_FEATURES
#define __OPENFLOW_SWITCH_FEATURES

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <string>
#include <inttypes.h>
#include "../../common/mac_address.h"
#include "../openflow_types/of_switch_capabilities.h"
#include "../openflow_types/of_actions_supported.h"

// structure used for dscribing switch features
typedef struct openflow_switch_features {
	void clear() {
		datapath_id = 0;
		n_buffers = 0;
		n_tables = 0;
		
		switch_address.clear();
		capabilities.clear();
		actions_supported.clear();
	}

	uint64_t	datapath_id;
	uint32_t	n_buffers;
	uint32_t	n_tables;

	mac_address							switch_address;
	of_switch_capabilities	capabilities;
	of_actions_supported		actions_supported;

	// generates a readable version of the object
	std::string to_string() const;

} openflow_switch_features;

#endif
