#include "switch_commander.h"
#include "../gui/output.h"
#include "../../common/timer.h"
#include "../../common/common_utils_oop.h"
#include <string>

// generates a readable version of the vlan_set object
string vlan_set::to_string() const {

	char buf[1024];
	string result;
	sprintf(buf, "vlan %hu\n"
		"description   : [%s]\n"
		"status        : [%s]\n",
		vlan_id,
		description.c_str(),
		active ? "active" : "inactive");

	result = buf;
	result += "tagged ports  : ";
	if (tagged_ports.empty()) {
		result += "[none]";
	} else {
		for (const auto& p : tagged_ports) {
			sprintf(buf, "%hu ", p);
			result += buf;
		}
	}

	result += "\nuntagged ports: ";
	if (untagged_ports.empty()) {
		result += "[none]";
	} else {
		for (const auto& p : untagged_ports) {
			sprintf(buf, "%hu ", p);
			result += buf;
		}
	}

	return result;
}

switch_commander::switch_commander() {
}

switch_commander::~switch_commander() {

}

// instantiates the switch telnet object and logs on to the switch management, then sets
// the terminal length
bool switch_commander::init(unique_ptr<iodevice> device,  const ip_port& address_and_port, const string& username, const string& password) {

	string cmd;

	// acquire device
	dev = move(device);
	if (dev == nullptr) {
		output::log(output::loglevel::ERROR, "switch_commander::init() did not get a valid device.\n");
		goto fail;
	}

	// set device specific timeout
	if (dev->get_device_type() == iodevice::device_type::SERIAL) {
		TIMEOUT_MS = TIMEOUT_SERIAL;
	} else if (dev->get_device_type() == iodevice::device_type::TELNET) {
		TIMEOUT_MS = TIMEOUT_TELNET;
	}

	// if the device is a telnet device, establish the login
	if (dev->get_device_type() == iodevice::device_type::TELNET) {
		switch_telnet* telnet_dev = static_cast<switch_telnet*>(dev.get());

		if (!telnet_dev->connect(address_and_port, username, password)) {
			output::log(output::loglevel::ERROR, "switch_commander::init() unable to connect/logon to switch.\n");
			goto fail;
		}
	}

	// disable terminal scrolling and wait for prompt
	return issue_command("terminal length 0\n", true);

fail:
	dev.reset();
	return false;
}

// shuts down switch commander
void switch_commander::shutdown() {
	dev.reset();
}

// gets the vlan settings for the switch
bool switch_commander::get_vlan_settings(int instance, vector<vlan_set>& result) {

	char response_buf[512*1024];
	int bytes_read;
	string response;
	char cmd_buf[64];
	char token_buf[512];
	string token;
	size_t offset_start, offset_end;

	result.clear();

	// sanity checks
	if (instance < 1 || instance > 8) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() invalid instance %d.\n", instance);
		return false;
	} else if (dev == nullptr) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() invalid device (dev = nullptr).\n");
		return false;
	}

	// wait for the command prompt
	if (!wait_for_prompt()) return false;

	// issue command to grab instance info
	sprintf(cmd_buf, "show openflow of-instance %d\n", instance);
	if (!issue_command(cmd_buf)) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not write show openflow of-instance command.\n");
		return false;
	}

	// read in of-instance data
	if (dev->read_until_timeout(response_buf, sizeof(response_buf)-1, TIMEOUT_MS, &bytes_read) == iodevice::status::DISCONNECTED) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not read response from show openflow of-instance, device failed.\n");
		return false;
	} else {
		response_buf[bytes_read] = 0;
		response = response_buf;
	}

	// look for token with the instance information
	sprintf(token_buf, "Instance        : %d", instance);
	if (response.find(token_buf) == string::npos) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not find instance token.\n");
		return false;
	}

	// look for vlan list
	token = " Vlan List      :";
	offset_start = response.find(token);
	if (offset_start == string::npos) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not find vlan list.\n");
		return false;
	} else {
		offset_start += token.size();
	}

	// look for Vlan Mbr list (so we know where to finish reading)
	offset_end = response.find("Vlan Mbr list");
	if (offset_end == string::npos) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not find vlan member token.\n");
		return false;
	}

	// grab everything until we see 'Vlan Mbr list', then tokenize and capture vlan numbers
	string vlan_list = response.substr(offset_start, offset_end-offset_start);
	vector<string> tokens = string_utils::tokenize(vlan_list, " ");

	bool parse_next_as_vlan = false;
	uint16_t vlan_id;
	for (const auto& token : tokens) {
		if (token == "Vl") {
			parse_next_as_vlan = true;
		} else if (parse_next_as_vlan) {	
			if (sscanf(token.c_str(), "%hu", &vlan_id) != 1) {
				output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() could not parse vlan [%s], original string is [%s]\n", token.c_str(), vlan_list.c_str());
				return false;
			} else {
				
				vlan_set new_set;
				new_set.vlan_id = vlan_id;
				result.push_back(new_set);
			}

			parse_next_as_vlan = false;
		}
	}

	// now get all vlan and port information
	vector<vlan_set> all_vlans;
	if (!get_all_vlans(all_vlans)) {
		output::log(output::loglevel::ERROR, "switch_commander::get_vlan_settings() failed to get vlan listing.\n");
		return false;
	}

	// filter results to keep only relevant vlans
	for (auto& dest : result) {
		for (const auto& vlan : all_vlans) {
			if (vlan.vlan_id == dest.vlan_id) {
				dest = vlan;
				break;
			}
		}
	}

	return true;
}

// gets information about all vlans on the switch
bool switch_commander::get_all_vlans(vector<vlan_set>& result) {

	char response_buf[512*1024];
	int bytes_read;
	size_t offset_start;
	vector<string> tokens;
	string response;
	result.clear();

	// read in vlan ports
	if (!wait_for_prompt()) {
		output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not get prompt.\n");
		return false;
	}

	// retrieve vlan to port mappings
	if (!issue_command("show vlan\n")) {
		output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not write 'show vlan' command.\n");
		return false;
	}

	// read in response
	if (dev->read_until_timeout(response_buf, sizeof(response_buf)-1, TIMEOUT_MS, &bytes_read) == iodevice::status::DISCONNECTED) {
		output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not read response from 'show vlan' command, device failed.\n");
		return false;
	} else {
		response_buf[bytes_read] = 0;
		response = response_buf;
	}

	// search for the the line of text that indicates the vlan/port data will follow next
	offset_start = response.find("Q Ports");
	if (offset_start == string::npos) {
		output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not find token for port results.\n");
		return false;
	}

	// tokenize by line
	tokens = string_utils::tokenize(response.substr(offset_start+8), "\n");
	bool vlan_active;
	string description;
	vlan_set current_vlan;
	uint16_t vlan_id;
	for (const auto& line : tokens) {
	
		if (line.size() < 18) continue;

		// read vlan number
		if (sscanf(line.c_str()+4, "%hu", &vlan_id) == 1) {

			// read in status
			char status_buf[128];
			string status_string;
			if (sscanf(line.c_str()+11, "%s", status_buf) != 1) {
				output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not parse status on line [%s]\n", line.c_str());
				return false;
			}
		
			status_string = status_buf;
			if (status_string != "Active" && status_string != "Inactive") {
				output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not parse status on line [%s]\n", line.c_str());
				return false;
			}
			vlan_active = (status_string == "Active");

			// read in description
			description = string_utils::trim_leading_and_trailing_whitespace(line.substr(21, 30));

			// setup this entry in the result vector
			vlan_set vlan;
			vlan.vlan_id = vlan_id;
			vlan.active = vlan_active;
			vlan.description = description;
			result.push_back(vlan);
		}

		// if the vlan is inactive, there's nothing else to do, so parse the next line of info
		if (!vlan_active) {
			continue;

		// vlan is active, so parse this line of port information
		} else if (line.size() > 54 && (line[53] == 'T' || line[53] == 'U')) {

			string port_listing = line.substr(60);
			
			set<uint16_t> port_list;
			if (!string_to_number_set(port_listing, port_list)) {
				output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not parse port listing: [%s]\n", line.c_str());
			} else if (line[53] == 'T') {
				result[result.size()-1].tagged_ports = port_list;
/*				output::printf("vlan %hu tagged ports:\n", result[result.size()-1].vlan_id);
				for (const auto& p : port_list) {
					output::printf("%hu, ", p);
				}
				output::printf("\n");
				*/
			} else {
				result[result.size()-1].untagged_ports = port_list;
/*				output::printf("vlan %hu untagged ports:\n", result[result.size()-1].vlan_id);
				for (const auto& p : port_list) {
					output::printf("%hu, ", p);
				}
				output::printf("\n");
				*/

			}

		// can't parse this line. maybe it's the command prompt
		} else if (line.size() > 0 && line[line.size()-1] == '#') {

			return true;

		// parse error
		} else {
			output::log(output::loglevel::ERROR, "switch_commander::get_all_vlans() could not parse port listing: [%s]\n", line.c_str());
			return false;
		}
	}

	return true;
}

// waits until the switch delivers the prompt
bool switch_commander::wait_for_prompt() {

	// wait for prompt
	timer wait_timer;
	char one_byte;
	int bytes_written;
	iodevice::status status;

	// issue a newline to force a prompt
	if (!dev->write("\n", 1)) {
		output::log(output::loglevel::ERROR, "switch_commander::wait_for_prompt() -- could not write to device.\n");
		return false;
	}

	while (1) {
		status = dev->read_until_timeout(&one_byte, 1, TIMEOUT_MS, &bytes_written);
		if (status == iodevice::status::DISCONNECTED) {
			output::log(output::loglevel::ERROR, "switch_commander::wait_for_prompt() -- could not read from device.\n");
			return false;
		} else if (status == iodevice::status::SUCCESS) {
			break;
		}
	}

	if (one_byte == '#' || one_byte == '>') {
		return true;
	} else {
		output::log(output::loglevel::ERROR, "switch_commander::wait_for_prompt() -- failed to get prompt (last byte is not '#' or '>').\n");
		return false;
	}
}

// issues a command, optionally waiting for the terminal prompt to reappear
bool switch_commander::issue_command(const string& cmd, bool wait) {
	
	if (!dev->write(cmd.c_str(), cmd.size())) {
		output::log(output::loglevel::ERROR, "switch_commander::issue_command() failed to issue command [%s].\n", cmd.c_str());
		return false;
	}

	if (wait) {
		return wait_for_prompt();
	}
	return true;
}

// converts a string of numbers, potentially in range format, into a set of uint16_t's.
bool switch_commander::string_to_number_set(const string& input, set<uint16_t>& result) {
	
	result.clear();
	vector<string> tokens = string_utils::tokenize(input, ",");
	uint16_t vlan_start, vlan_end;
	for (const auto& token : tokens) {

		// handle individual ports
		if (token.find("-") == string::npos) {
			if (sscanf(token.c_str(), "%hu", &vlan_start) == 1) {
				result.insert(vlan_start+1);	// add one for openflow numbering
			} else {
				output::log(output::loglevel::ERROR, "switch_commander::string_to_number_set() could not parse token [%s]\n", token.c_str());
				return false;
			}

		// handle port range
		} else {
			if (sscanf(token.c_str(), "%hu-%hu", &vlan_start, &vlan_end) == 2) {
				for (uint16_t counter = vlan_start; counter <= vlan_end; ++counter) {
					result.insert(counter+1);		// add one for openflow numbering
				}
			} else {
				output::log(output::loglevel::ERROR, "switch_commander::string_to_number_set() could not parse token [%s]\n", token.c_str());
				return false;
			}
		}
	}

	return true;
}
