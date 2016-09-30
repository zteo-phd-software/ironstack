#include "serial.h"

// constructor
serial_port::serial_port() {
	initialized = false;
	baudrate = 9600;
	fd = -1;
}

// shortcut constructor
serial_port::serial_port(const string& dev, uint32_t brate) {
	initialized = false;
	device = dev;
	baudrate = brate;
	fd = -1;
}

// destructor
serial_port::~serial_port() {
	shutdown();
}

// get the device type (serial)
iodevice::device_type serial_port::get_device_type() const {
	return device_type::SERIAL;
}

// sets up the device name. cannot do this once serial port is opened.
bool serial_port::set_device(const string& dev) {
	if (initialized) {
		return false;
	}
	device = dev;
	return true;
}

// sets the baud rate for the device. cannot do this once serial port is
// opened.
bool serial_port::set_baud_rate(uint32_t baud) {
	if (initialized) {
		return false;
	}
	
	if (!validate_baud_rate(baud)) {
		return false;
	}

	baudrate = baud;
	return true;
}

// opens the device. cannot do this while serial port is opened. (need
// to close the current device first before opening another).
bool serial_port::init() {
	fd = open(device.c_str(), O_RDWR | O_NOCTTY);
	if (fd == -1) {
		printf("serial_port::init() failed -- did you run with sudo?\n");
		return false;
	}

	fcntl(fd, F_SETFL, 0);

	struct termios options;

	// translate to speed required
	switch (baudrate) {
		case 9600:
			options.c_cflag = B9600;
			break;
		case 19200:
			options.c_cflag = B19200;
			break;
		case 38400:
			options.c_cflag = B38400;
			break;
		case 57600:
			options.c_cflag = B57600;
			break;
		case 115200:
			options.c_cflag = B115200;
			break;
		default:
			printf("serial_port::init() error -- invalid/unsupported baud rate "
				"specified.\n");
			return false;
	}

	options.c_cflag |= CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;	// no parity
	options.c_oflag = 0;		
	options.c_lflag = 0;			// set to non canonical (ie. raw) mode and no echo
	options.c_cc[VTIME] = 0;	// disable timeout
	options.c_cc[VMIN] = 0;		// disable minimum chars to read

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);	// change it now
	initialized = true;
	return true;
}

// check to see if a port has data ready within some specified time
bool serial_port::wait_for_input(uint32_t milliseconds) {
	fd_set read_set;
	FD_SET(fd, &read_set);
	struct timeval timeout;
	timeout.tv_sec = milliseconds/1000;
	timeout.tv_usec = (milliseconds - (timeout.tv_sec * 1000)) * 1000;
	
	int result = select(fd+1, &read_set, NULL, NULL, &timeout);
	return result > 0;
}

// closes the current device (but retains current settings)
void serial_port::shutdown() {
	if (initialized) {
		close(fd);
		fd = -1;
		initialized = false;
	}
}

// reads the device
// returns number of bytes read
int serial_port::read(autobuf& dest, uint32_t max_size) {
	if (!initialized) {
		return -1;
	}

	uint8_t buf[max_size];
	int result = ::read(fd, buf, max_size);
	if (result == -1) {
		printf("serial_port::read() error. %s\n", strerror(errno));
		return 0;
	}

	dest.set_content(buf, result);
	return result;
}

// reads the device into a raw buffer
int serial_port::read(void* buf, uint32_t max_size) {
	autobuf dest;
	dest.inherit_shared(buf, max_size, max_size);
	return read(dest, max_size);
}

// writes to the device
// returns the number of bytes written
int serial_port::write(const autobuf& src) {
	if (!initialized) {
		return -1;
	}

	uint32_t bytes_written = 0;
	int result = 0;
	
	while (bytes_written != src.size()) {

		result = ::write(fd, src.ptr_offset_const(bytes_written),
			src.size()-bytes_written);
		if (result == -1) {
			printf("serial_port::write() error.\n");
			return 0;
		}

		bytes_written += result;
	}

	return bytes_written;
}

// writes to the device from a raw buffer
int serial_port::write(const void* buf, uint32_t max_size) {
	autobuf src;
	src.inherit_read_only(buf, max_size);
	return write(src);
}

// checks to make sure the baud rate selected is allowable
bool serial_port::validate_baud_rate(uint32_t baud) {

	switch (baud) {

		case 9600:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
			return true;

		default:
			return false;
	}
}
