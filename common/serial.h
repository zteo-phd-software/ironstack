#pragma once

#include "iodevice.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
using namespace std;

/* 
 * serial port utilities.
 *
 * this is a simplified serial port access class.
 * supply a serial device name and call open_port(). the only changeable
 * parameters are baud rate (data defaults to 8N1) and timeout.
 *
 * note: the functions in this class are not thread-safe. implement
 * thread safety on your own.
 *
 */
class serial_port final : public iodevice {
public:

	// constructors and destructor
	serial_port();
	serial_port(const string& device, uint32_t baudrate);
	virtual ~serial_port();

	// disable copying and assignment operator
	serial_port(const serial_port& other) = delete;
	serial_port& operator=(const serial_port& other) = delete;

	// get the device type (serial)
	virtual device_type get_device_type() const;

	// alternative ways to set port and baud
	bool set_device(const string& device);
	bool set_baud_rate(uint32_t baudrate);

	// I/O functions
	virtual bool init();
	virtual void shutdown();

	// lets the caller know if input is ready within some time
	// this can be used to implement timeout checks
	// if timeout is set to 0ms, it returns immediately.
	virtual bool wait_for_input(uint32_t milliseconds);

	virtual int read(autobuf& dest, uint32_t max_size=128);
	virtual int read(void* buf, uint32_t max_size);

	virtual int write(const autobuf& src);
	virtual int write(const void* buf, uint32_t size);

	// used to check state
	string get_device_name() const { return device; }
	uint32_t get_baud_rate() const { return baudrate; }

	// advanced use only
	int get_fd() { return fd; }

private:

	bool validate_baud_rate(uint32_t baud);

	string device;
	uint32_t baudrate;
	int fd;

};
