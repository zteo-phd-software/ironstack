#pragma once
#include "autobuf.h"
#include <stdint.h>

/*
 * this abstract class defines an interface for an I/O device.
 *
 * this was meant to provide an abstract interface to communicate with a
 * force10 switch through telnet or serial.
 *
 */
class iodevice {
public:

	enum class device_type { SERIAL, TELNET };
	enum class status { SUCCESS, DISCONNECTED, BUFFER_FULL, TIMED_OUT };

	iodevice():initialized(false) {}
	~iodevice() {}

	// startup and shutdown functions
	virtual bool init()=0;
	virtual void shutdown()=0;

	// get the device type
	virtual device_type get_device_type() const=0;

	// timeout functions
	// timeout of 0 means the function should return immediately
	virtual bool wait_for_input(uint32_t milliseconds)=0;

	// read functions
	virtual int read(autobuf& dest, uint32_t max_size=128)=0;
	virtual int read(void* dest, uint32_t max_size)=0;

	// keeps reading the device until timeout_ms has elapsed and no more chars are available
	// returns: SUCCESS if timeout period elapsed (regardless of whether bytes were read or not)
	//          DISCONNECTED if device was disconnected
	//          BUFFER_FULL if the buffer filled up before the timeout period occurred
	virtual status read_until_timeout(autobuf& dest, uint32_t max_size, uint32_t timeout_ms)=0;
	virtual status read_until_timeout(void* dest, uint32_t max_size, uint32_t timeout_ms, int* buf_used)=0;

	// write functions
	virtual int write(const autobuf& src)=0;
	virtual int write(const void* buf, uint32_t size)=0;

	// check if the iodevice is live
	bool is_init() const { return initialized; }

protected:

	bool initialized;
};


