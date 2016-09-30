#include "switch_telnet.h"

// default constructor
switch_telnet::switch_telnet() {
	logged_in = false;
}

// gets the switch device type (telnet)
iodevice::device_type switch_telnet::get_device_type() const {
	return iodevice::device_type::TELNET;
}

// connect to the switch
bool switch_telnet::connect(const ip_port& address_and_port,
	const string& username, const string& password) {

	logged_in = false;
	if (!connection.connect(address_and_port)) {
		printf("switch_telnet::connect() error -- unable to connect.\n");
		return false;
	}

	// negotiate telnet parameters
	autobuf buf;
	struct timeval current_time, start_time, difference_time;
	int ms_left;
	autobuf read_buffer;
	string result;

	while(1) {
		if (!connection.recv_fixed_bytes(buf, 1)) goto io_failed;
		if (buf[0] != 255) break;
	

		if (!connection.recv_fixed_bytes(buf, 1)) goto io_failed;
		switch (buf[0]) {
			case 253:	// DO command
			{
				char response[3];
				if (!connection.recv_fixed_bytes(buf, 1)) goto io_failed;
				response[0] = 255;		// command escape mode
				response[1] = 252;		// won't
				response[2] = buf[0];	// whatever the server asked

				if (!connection.send_raw(response, 3)) goto io_failed;
				break;
			}
			case 254:	// DONT command
			case 251: // WILL response
			case 252: // WONT response
			{
				if (!connection.recv_fixed_bytes(buf, 1)) goto io_failed;
				break;
			}
			default:
			{
				if (!connection.recv_fixed_bytes(buf, 1)) goto io_failed;
				printf("telnet: unhandled command %d.\n", buf[0]);
				break;
			}
		}
	}

	// finished negotiating, now look for the login prompt
	while (connection.recv_fixed_bytes(buf, 1) && buf[0] != ':');

	// send username
	if (!connection.send_raw(username.c_str(), username.size())
		|| !connection.send_raw("\n", 1)) goto io_failed;

	// wait for password prompt
	while (connection.recv_fixed_bytes(buf, 1) && buf[0] != ':');

	// send password
	if (!connection.send_raw(password.c_str(), password.size())
		|| !connection.send_raw("\n", 1)) goto io_failed;

	gettimeofday(&start_time, nullptr);
	while(1) {

		// check for timeout (5 seconds)
		gettimeofday(&current_time, nullptr);
		timersub(&current_time, &start_time, &difference_time);
		ms_left = 5000-difference_time.tv_sec*1000 + difference_time.tv_usec/1000;
		if (ms_left < 0 || !connection.recv_fixed_bytes(buf, 1, ms_left)) {
			printf("switch_telnet::connect() error -- authentication timeout.\n");
			goto failed;
		}

		// append to read buffer
		read_buffer += buf;
		result = read_buffer.to_string();
		if (result.find("Authentication failed") != string::npos) {
			printf("switch_telnet::connect() error -- authentication failed.\n");
			goto failed;
		} else if (result.find("#") != string::npos
			|| result.find(">") != string::npos) {
			break;
		}
	}

	// login succeeded
	logged_in = true;
	initialized = true;
	return true;

io_failed:
	printf("switch_telnet::connect() -- socket I/O error.\n");

failed:
	connection.close();
	return false;
}

// initializes the device
bool switch_telnet::init() {
	return logged_in;
}

// shuts down the connection
void switch_telnet::shutdown() {
	logged_in = false;
	connection.close();
}

// checks if there is input waiting
bool switch_telnet::wait_for_input(uint32_t milliseconds) {
	if (!logged_in) return false;

	int fd = connection.get_fd();
	fd_set read_set;
	FD_SET(fd, &read_set);
	struct timeval timeout;
	timeout.tv_sec = milliseconds/1000;
	timeout.tv_usec = (milliseconds - (timeout.tv_sec*1000)) * 1000;

	int result = select(fd+1, &read_set, nullptr, nullptr, &timeout);
	return result > 0;
}

// read from the device
int switch_telnet::read(autobuf& dest, uint32_t max_size) {
	
	if (!logged_in) return -1;

	int bytes_used;
	char buf[max_size];
	if (!connection.recv_raw(buf, &bytes_used, max_size)) {
		shutdown();
		return -1;
	}

	if (bytes_used == 0) {
		dest.clear();
	} else {
		dest.set_content(buf, bytes_used);
	}

	return bytes_used;
}

// read from the device
int switch_telnet::read(void* buf, uint32_t max_size) {

	if (!logged_in) return -1;
	
	int bytes_used;
	if (!connection.recv_raw(buf, &bytes_used, max_size)) {
		shutdown();
		return -1;
	}

	return bytes_used;
}

// reads from the device until a certain timeout has elapsed
iodevice::status switch_telnet::read_until_timeout(autobuf& dest,
	uint32_t max_size, uint32_t timeout_ms) {

	char buf[max_size];
	int buf_used = 0;
	iodevice::status result = read_until_timeout(buf, max_size,
		timeout_ms, &buf_used);
	dest.set_content(buf, buf_used);
	return result;
}

// reads from the device until a certain timeout has elapsed
iodevice::status switch_telnet::read_until_timeout(void* buf, uint32_t max_size,
	uint32_t timeout_ms, int* buf_used) {

	int bytes_used = 0;
	uint32_t offset = 0;
	iodevice::status result = iodevice::status::SUCCESS;
	if (max_size == 0) return iodevice::status::BUFFER_FULL;

	while (wait_for_input(timeout_ms)) {
		if (!connection.recv_raw((char*)buf+offset, &bytes_used, 1)) {
			result = iodevice::status::DISCONNECTED;
			goto done;
		} else {
			offset += bytes_used;
		}

		if (offset >= max_size) {
			result = iodevice::status::BUFFER_FULL;
			goto done;
		}
	}

done:
	*buf_used = offset;
	return result;
}

// writes to the device
int switch_telnet::write(const autobuf& src) {

	if (!logged_in) return -1;
	if (connection.send_raw(src)) {
		return src.size();
	} else {
		shutdown();
		return -1;
	}
}

// writes to the device
int switch_telnet::write(const void* buf, uint32_t size) {

	if (!logged_in) return -1;
	if (connection.send_raw(buf, size)) {
		return size;
	} else {
		shutdown();
		return -1;
	}
}

