#pragma once

#include "iodevice.h"
#include "ip_address.h"
#include "ip_port.h"
#include "tcp.h"
#include <string>
using namespace std;

/*
 * telnet client utility.
 * this provides telnet access to a dell s4810 switch,
 * similar to the serial port utility.
 *
 * note: the functions in this class are not thread-safe.
 */

class switch_telnet final : public iodevice {
public:

	switch_telnet();

	// disable copying and assignment operator
	switch_telnet(const switch_telnet& other) = delete;
	switch_telnet& operator=(const switch_telnet& other) = delete;

	// get the device type (ie, telnet for this class)
	virtual device_type get_device_type() const;

	// connect and login to the switch
	bool connect(const ip_port& address_and_port, const string& username, const string& password);

	// I/O functions
	virtual bool init();
	virtual void shutdown();

	// lets the caller know if input is ready within some time bound
	// useful for timeout checks. if timeout is set to 0ms, function
	// returns immediately
	virtual bool wait_for_input(uint32_t milliseconds);

	virtual int read(autobuf& dest, uint32_t max_size=128);
	virtual int read(void* buf, uint32_t max_size);

	virtual status read_until_timeout(autobuf& dest, uint32_t max_size, uint32_t timeout_ms);
	virtual status read_until_timeout(void* dest, uint32_t max_size, uint32_t timeout_ms, int* buf_used);

	virtual int write(const autobuf& src);
	virtual int write(const void* buf, uint32_t size);

private:

	static const uint32_t TIMEOUT_MS = 3000;
	bool logged_in;
	tcp connection;
};
