#include "tcp.h"

// static initializers here
mutex tcp::lock;
tcp_global_info tcp::global_info;
map<uint16_t, int> tcp::listening_port_to_socket_mapping;

// constructor for the tcp object
tcp::tcp() {
	socket_id = -1;
}

// destructor for tcp object
tcp::~tcp() {
	close();
}

// makes a tcp connection
bool tcp::connect(const ip_port& address_and_port) {

	// close any existing connection
	close();
	connection_info.clear();

	// validate given input
	if (address_and_port.is_nil()) {
		printf("tcp::conect() error -- invalid address/port.\n");
		return false;
	}
	
	// create socket
	socket_id = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_id < 0) {
		printf("tcp::connect() failed to create socket. reason: %s\n", strerror(errno));
		return false;
	}

	struct sockaddr_in remote_sockaddr;
	remote_sockaddr.sin_family = AF_INET;
	remote_sockaddr.sin_addr.s_addr = address_and_port.get_address().get_as_be32();
	remote_sockaddr.sin_port = htons(address_and_port.get_port());

	// make the connection
	if (::connect(socket_id, (struct sockaddr*) &remote_sockaddr, sizeof(remote_sockaddr)) < 0) {
		if (errno != ENETUNREACH && errno != ETIMEDOUT) {
			printf("tcp::connect() failed to connect. reason: %s\n", strerror(errno));
		}
		::close(socket_id);
		socket_id = -1;
		return false;
	}

	// store local ip address/port
	struct sockaddr_in local_sockaddr;
	socklen_t addr_len = sizeof(local_sockaddr);
	if (getsockname(socket_id, (struct sockaddr*) &local_sockaddr, &addr_len) < 0) {
		printf("tcp::connect() failed to get local sockaddr. reason: %s\n", strerror(errno));
		::close(socket_id);
		socket_id = -1;
		return false;
	}
	connection_info.local_ip_address.set(inet_ntoa(local_sockaddr.sin_addr));
	connection_info.local_port = ntohs(local_sockaddr.sin_port);

	// store remote ip address/port
	connection_info.remote_ip_address = address_and_port.get_address();
	connection_info.remote_port = address_and_port.get_port();

	// update other metadata
	connection_info.connected = true;
	connection_info.bytes_sent = 0;
	connection_info.bytes_received = 0;
	gettimeofday(&connection_info.connection_start_time, nullptr);
	global_info.connections_active++;

	return true;
}

// makes a connection, using old style
bool tcp::connect(const string& address, uint16_t port) {
	ip_port address_and_port(address, port);
	return connect(address_and_port);
}

// closes a tcp connection
void tcp::close() {
	if (connection_info.is_connected()) {
		::close(socket_id);
		socket_id = -1;
		
		connection_info.connected = false;
		global_info.connections_active--;
	}
}

// enables listening on a port, does not accept the next incoming connection
bool tcp::listen(uint16_t port) {
	// if there's already a listen activity on the port, don't do listen() again
	lock_guard<mutex> g(lock);
	if (listening_port_to_socket_mapping.count(port) > 0) {
		return false;
	}

	// create a socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		printf("tcp::listen() failed -- socket creation failed. reason: %s\n", strerror(errno));
		return false;
	}

	// set to reusable
	struct sockaddr_in local_sockaddr;
	int one = 1;
	local_sockaddr.sin_family = AF_INET;
	local_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_sockaddr.sin_port = htons(port);
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
		printf("tcp::listen() failed -- could not bind to socket. reason: %s\n", strerror(errno));
		::close(server_socket);
		return false;
	}

	// bind socket to local sockaddr_in
	if (bind(server_socket, (struct sockaddr*) &local_sockaddr, sizeof(local_sockaddr)) == -1) {
		printf("tcp::listen() failed -- could not bind to socket. reason: %s\n", strerror(errno));
		::close(server_socket);
		return false;
	}

	// set to listen mode
	if (::listen(server_socket, 15) == -1) {
		printf("tcp::listen() failed -- could not listen on socket. reason: %s\n", strerror(errno));
		::close(server_socket);
		return false;
	}

	// insert to maps
	listening_port_to_socket_mapping[port] = server_socket;
	global_info.listening_ports[port] = true;
	return true;
}

// stops listening on a port
void tcp::stop_listen(uint16_t port) {
	lock_guard<mutex> g(lock);
	auto iterator = listening_port_to_socket_mapping.find(port);
	if (iterator != listening_port_to_socket_mapping.end()) {
		::close(iterator->second);
		listening_port_to_socket_mapping.erase(iterator);
		
		auto iterator2 = global_info.listening_ports.find(port);
		if (iterator2 != global_info.listening_ports.end()) {
			global_info.listening_ports.erase(iterator2);
			return;
		}
	}
}

// accepts a connection into this tcp connection
bool tcp::accept(uint16_t port) {

	// close any existing conection
	close();
	connection_info.clear();

	// look up the listening socket
	int listening_socket = -1;
	{
		lock_guard<mutex> g(lock);
		auto iterator = listening_port_to_socket_mapping.find(port);
		if (iterator == listening_port_to_socket_mapping.end()) {
			socket_id = -1;
			return false;
		}
		listening_socket = iterator->second;
	}

	// try to accept the connection
	struct sockaddr_in remote_sockaddr;
	struct sockaddr_in local_sockaddr;
	socklen_t address_len = sizeof(remote_sockaddr);

	socket_id = ::accept(listening_socket, (struct sockaddr*) &remote_sockaddr, &address_len);
	if (socket_id == -1) {
		printf("tcp::accept() error -- failed to accept. reason: %s\n", strerror(errno));
		return false;
	}

	// successfully connected. get local information
	address_len = sizeof(local_sockaddr);
	if (getsockname(socket_id, (struct sockaddr*) &local_sockaddr, &address_len) < 0) {
		printf("tcp::connect() failed to get local sockaddr. reason: %s\n", strerror(errno));
		::close(socket_id);
		socket_id = -1;
		return false;
	}

	// populate metadata
	connection_info.local_ip_address.set(inet_ntoa(local_sockaddr.sin_addr));
	connection_info.local_port = ntohs(local_sockaddr.sin_port);
	connection_info.remote_ip_address.set(inet_ntoa(remote_sockaddr.sin_addr));
	connection_info.remote_port = ntohs(remote_sockaddr.sin_port);
	connection_info.connected = true;
	connection_info.bytes_sent = 0;
	connection_info.bytes_received = 0;
	gettimeofday(&connection_info.connection_start_time, nullptr);
	global_info.connections_active++;

	return true;
}

// sets the name for this tcp connection
void tcp::set_name(const string& _name) {
	name = _name;
}

// gets the name of this tcp connection
string tcp::get_name() const {
	return name;
}

// sends an automemory without block delimiter
bool tcp::send_raw(const autobuf& payload) {
	return send_raw(payload.get_content_ptr(), payload.size());
}

// sends a binary buffer without block delimiter
bool tcp::send_raw(const void* buf, int len) {

	// sanity check
	if (!connection_info.is_connected()) {
		return false;
	}

	// setup send loop
	int total_bytes_sent = 0;
	int current_bytes_sent;
	int bytes_to_send = 0;

	while (total_bytes_sent < len) {
		bytes_to_send = (len - total_bytes_sent) >= 1024 ? 1024 : len - total_bytes_sent;

		while ((current_bytes_sent = ::send(socket_id, ((char*)buf)+total_bytes_sent,
			bytes_to_send, MSG_NOSIGNAL)) == -1 && errno == EINTR);

		if (current_bytes_sent <= 0) {
			// permanent error or client disconnected
			close();
			return false;
		}
		total_bytes_sent += current_bytes_sent;
		connection_info.bytes_sent += current_bytes_sent;			// possible contention?
		global_info.total_bytes_sent += current_bytes_sent;		// possible contention?
	}

	return true;
}

// receives data into an automemory payload, no block delimiter
bool tcp::recv_raw(autobuf& payload, int timeout_ms) {

	payload.clear();
	if (!connection_info.is_connected()) {
		return false;
	}

	if (timeout_ms >= 0 && !timeout_check(timeout_ms)) {
		return false;
	}

	payload.create_empty_buffer(1024, false);
	int bytes_received;
	while((bytes_received = ::recv(socket_id, payload.get_content_ptr_mutable(), 1024, 0)) == -1 && errno == EINTR);
	if (bytes_received <= 0) {
		// permanent error or client disconnected
		close();
		return false;
	}

	payload.create_empty_buffer(bytes_received, false);
	connection_info.bytes_received += bytes_received;
	global_info.total_bytes_received += bytes_received;

	return true;
}

// receives data into a binary buffer, no block delimiter
bool tcp::recv_raw(void* buf, int* buf_used, int len, int timeout_ms) {

	if (!connection_info.is_connected()) {
		return false;
	}

	if (timeout_ms >= 0 && !timeout_check(timeout_ms)) {
		return false;
	}

	int bytes_received = ::recv(socket_id, buf, len, 0);
	if (bytes_received <= 0) {
		close();
		return false;
	}

	*buf_used = bytes_received;
	connection_info.bytes_received += bytes_received;
	global_info.total_bytes_received += bytes_received;

	return true;
}

// receives a fixed number of bytes from the socket or fail trying
bool tcp::recv_fixed_bytes(void* buf, int bytes_to_read, int timeout_ms) {
	if (!connection_info.is_connected()) {
		return false;
	}
	
	bool is_blocking = (timeout_ms == -1);
	struct timeval start_time, current_time, difference_time;
	gettimeofday(&start_time, nullptr);

	int total_bytes_read = 0;
	int current_bytes_read = 0;
	int ms_elapsed;
	int ms_remaining;
	while(total_bytes_read < bytes_to_read) {

		if (!is_blocking) {
			gettimeofday(&current_time, nullptr);
			timersub(&current_time, &start_time, &difference_time);
			ms_elapsed = difference_time.tv_sec*1000 + difference_time.tv_usec/1000;
			ms_remaining = timeout_ms - ms_elapsed;
			if (ms_remaining < 0) return false;
			if (!timeout_check(ms_remaining)) return false;
		}

		current_bytes_read = recv(socket_id, ((char*)buf)+total_bytes_read, bytes_to_read-total_bytes_read, 0);
		if (current_bytes_read == -1 && errno == EINTR) {
			continue;
		} else if (current_bytes_read <= 0) {
			close();
			return false;
		}
		
		total_bytes_read += current_bytes_read;
		connection_info.bytes_received += current_bytes_read;
		global_info.total_bytes_received += current_bytes_read;
	}

	return true;
}

// read in a fixed number of bytes from the socket or fail trying
bool tcp::recv_fixed_bytes(autobuf& buf, int bytes_to_read, int timeout_ms) {

	buf.create_empty_buffer(bytes_to_read, false);
	bool status = recv_fixed_bytes(buf.get_content_ptr_mutable(), bytes_to_read, timeout_ms);

	if (!status) buf.clear();
	return status;
}

// reads until a token is located in the input
bool tcp::recv_until_token(autobuf& buf, const autobuf& token, int timeout_ms) {

	buf.clear();
	if (!connection_info.is_connected()) {
		return false;
	}

	if (token.size() == 0) {
		return true;
	}

	bool is_blocking = (timeout_ms == -1);

	// determine minimum read size
	int read_size = token.size();
	char current_char;
	bool mismatch;
	int bytes_read = 0;
	struct timeval start_time, current_time, difference_time;
	gettimeofday(&start_time, nullptr);
	int ms_elapsed, ms_remaining;

	// read the first block
	if (!recv_fixed_bytes(buf, read_size)) {
		close();
		return false;
	}

	while(1) {

		// check buffer tail for substring
		mismatch = false;
		for (int counter = 0; counter < read_size; ++counter) {
			if (buf[buf.size()-token.size()+counter] != token[counter]) {
				mismatch = true;
				break;
			}
		}

		if (!mismatch) {
			break;
		}

		// check for timeout if applicable
		if (!is_blocking) {
			gettimeofday(&current_time, nullptr);
			timersub(&current_time, &start_time, &difference_time);
			ms_elapsed = difference_time.tv_sec*1000 + difference_time.tv_usec/1000;
			ms_remaining = timeout_ms - ms_elapsed;
			if (ms_remaining < 0) return false;
			if (!timeout_check(ms_remaining)) return false;
		}

		// read one more char
		bytes_read = ::recv(socket_id, &current_char, 1, 0);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				close();
				return false;
			}
		} else {
			buf.append(&current_char, 1);
			++connection_info.bytes_received;
			++global_info.total_bytes_received;
		}
	}

	return true;
}

// reads until a token is located in the input, token supplied as a raw byte buffer
bool tcp::recv_until_token(autobuf& buf, const void* token, int token_size, int timeout_ms) {
	autobuf _token;
	_token.set_content(token, token_size);
	return recv_until_token(buf, _token, timeout_ms);
}

// returns the raw socket descriptior
int tcp::get_fd() const {
	return socket_id;
}

// gets information about the current connection
tcp_info tcp::get_connection_info() const {

	return connection_info;
}

// tcp global information copy constructor
tcp_global_info::tcp_global_info(const tcp_global_info& other) {
	connections_active = other.connections_active.load();
	listening_ports = other.listening_ports;
	total_bytes_sent = other.total_bytes_sent.load();
	total_bytes_received = other.total_bytes_received.load();
}

// gets global statistics about the tcp subsystem
tcp_global_info tcp::get_global_info() {
	tcp_global_info result;
	lock_guard<mutex> g(lock);
	return global_info;
}

// gets a copy of the ports being listened on
vector<uint16_t> tcp_global_info::get_listening_ports() const {

	vector<uint16_t> result;
	result.reserve(listening_ports.size());

	for (const auto& it : listening_ports) {
		result.push_back(it.first);
	}

	return result;
}

// constructor
epoll_set::epoll_set() {
	fd = -1;
	init();
}

// destructor
epoll_set::~epoll_set() {
	close(fd);
}

// clear the epoll set
void epoll_set::clear() {
	init();
}

// adds a socket to the epoll set
void epoll_set::add(const shared_ptr<tcp>& connection) {

	if (connection == nullptr) {
		printf("epoll_set::add() error -- tried to add an invalid connection.\n");
		return;
	}

	int socket_id;
	if ((socket_id = connection->get_fd()) == -1) {
		return;
	}

	struct epoll_event event;
	event.data.fd = socket_id;
	event.events = EPOLLIN | EPOLLRDHUP;

	if (epoll_ctl(fd, EPOLL_CTL_ADD, socket_id, &event) == -1) {
		printf("epoll_set::add() error -- epoll_ctl() error: %s.\n", strerror(errno));
		abort();
	}

	lock_guard<mutex> g(lock);
	watch_set.insert(connection);
}

// removes a socket from the epoll set
void epoll_set::remove(const shared_ptr<tcp>& connection) {

	if (connection == nullptr) {
		printf("epoll_set::remove() error -- tried to remove an invalid connection.\n");
		return;
	}

	lock_guard<mutex> g(lock);
	shared_ptr<tcp> current;
	int socket_id = connection->get_fd();

	auto iterator = watch_set.begin();
	while (iterator != watch_set.end()) {
		if (iterator->unique() || *iterator == connection) {
			iterator = watch_set.erase(iterator);

			if (epoll_ctl(fd, EPOLL_CTL_DEL, socket_id, nullptr) == -1) {
				printf("epoll_set::remove() error -- epoll_ctl() error: %s.\n", strerror(errno));
				abort();
			}

		} else {
			++iterator;
		}
	}
}

// checks for the sockets that are available for work
set<shared_ptr<tcp>> epoll_set::wait(int timeout_ms) {

	struct epoll_event events[64];
	set<shared_ptr<tcp>> result;
	int ret;

	if ((ret = epoll_wait(fd, events, sizeof(events)/sizeof(struct epoll_event), timeout_ms)) == -1) {
		printf("epoll_set::wait() error -- epoll_wait() error: %s.\n", strerror(errno));
		abort();
	}
	
	// grab the list of sockets that have events
	set<int> active_fd;
	for (int counter = 0; counter < ret; ++counter) {
		active_fd.insert(events[counter].data.fd);
	}

	lock_guard<mutex> g(lock);
	auto iterator = watch_set.begin();
	while (iterator != watch_set.end()) {
		if (iterator->unique()) {
			if (epoll_ctl(fd, EPOLL_CTL_DEL, (*iterator)->get_fd(), nullptr) == -1) {
				printf("epoll_set::remove() error -- epoll_ctl() error: %s.\n", strerror(errno));
				abort();
			}
			iterator = watch_set.erase(iterator);

			continue;
		} else if (active_fd.count((*iterator)->get_fd()) > 0) {
			result.insert(*iterator);
		}
		++iterator;
	}

	return result;
}

// used to create a new fd for the epoll set
void epoll_set::init() {
	lock_guard<mutex> g(lock);

	if (fd != -1) {
		close(fd);
	}

	fd = epoll_create1(0);
	if (fd == -1) {
		printf("epoll_set() construction error -- %s\n", strerror(errno));
		abort();
	}
}

// used to check if a timeout occurred on the socket
bool tcp::timeout_check(int timeout_ms) {

	int seconds = timeout_ms / 1000;
	int useconds = (timeout_ms - (seconds*1000)) * 1000;

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(socket_id, &read_set);

	fd_set error_set;
	FD_ZERO(&error_set);
	FD_SET(socket_id, &error_set);

	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = useconds;

	return (select(socket_id+1, &read_set, nullptr, &error_set, &tv) > 0);
}
