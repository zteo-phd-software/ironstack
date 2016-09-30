#pragma once
#include "../../common/autobuf.h"
#include "../../common/openflow.h"
#include <string>
using namespace std;

// externally usable class
class openflow_switch_description {
public:

	// constructor
	openflow_switch_description() {}

	// clears the object
  void clear() { manufacturer.clear();
  	software_description.clear();
  	serial_number.clear();
    general_description.clear();
  }

	// generates debug string information
	string to_string() const { return 
		string("manufacturer : ") + manufacturer +
		string("\nhw desc      : ") + hardware_description +
		string("\nsw desc      : ") + software_description +
		string("\nserial       : ") + serial_number +
		string("\ngeneral desc : ") + general_description;
	}

	// serialization and deserialization
	uint32_t serialize(autobuf& dest) const;
	bool deserialize(const autobuf& input);

	// publicly accessible fields
  string manufacturer;
  string hardware_description;
  string software_description;
  string serial_number;
  string general_description;

};
