#pragma once
#include <memory>
#include <set>
#include <vector>
#include "../../common/switch_telnet.h"
using namespace std;

/*
 * this class describes a vlan set, which contains information about the
 * tagged/untagged ports for a given vlan ID.
 *
 */
class vlan_set {
public:
	uint16_t      vlan_id;
	bool          active;
	string        description;
	set<uint16_t> tagged_ports;
	set<uint16_t> untagged_ports;

	// displays a readable version of the object
	string        to_string() const;
};

/*
 * this class provides functionality to obtain switch vlan and port
 * configuration on startup.
 *
 * some limited utilities are also present to perform basic tasks.
 *
 */
class switch_commander {
public:

	switch_commander();
	~switch_commander();

	// startup and shutdown functions
	bool init(unique_ptr<iodevice> device, const ip_port& address_and_port, const string& username, const string& password);
	void shutdown();

	// get the list of vlans applied to an instance
	bool get_vlan_settings(int instance, vector<vlan_set>& result);

	// get information about all vlans on the switch
	bool get_all_vlans(vector<vlan_set>& result);

private:

	// pointer to either serial or telnet device
	unique_ptr<iodevice> dev;

	// helper functions
	bool wait_for_prompt();	// waits till the terminal stops spewing text and looks for the # sign
	bool issue_command(const string& cmd, bool wait=false);	// issues a command. set wait to wait for prompt
	bool string_to_number_set(const string& input, set<uint16_t>& result);

	// define some timeout numbers
	int TIMEOUT_MS;
	static const int TIMEOUT_SERIAL = 1000;
	static const int TIMEOUT_TELNET = 200;
};
