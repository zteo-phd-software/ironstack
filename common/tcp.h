#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>
#include "autobuf.h"
#include "ip_port.h"

// class to handle TCP functionality
// updated 3/2/15
// revision by Z. Teo

class tcp;
using namespace std;

// metadata information pertaining to each tcp connection
class tcp_info {
public:

	// constructors
	tcp_info() { clear(); }
	tcp_info(const ip_address& local_address,
		const ip_address& remote_address,
		uint16_t local_port_,
		uint16_t remote_port_):
		connected(false),
		local_ip_address(local_address),
		remote_ip_address(remote_address),
		local_port(local_port_),
		remote_port(remote_port_),
		bytes_sent(0),
		bytes_received(0) {

		memset(&connection_start_time, 0, sizeof(connection_start_time));
	}

	// clears the information
	void clear() {
		connected = false;
		local_ip_address.clear();
		remote_ip_address.clear();
		local_port = 0;
		remote_port = 0;
		bytes_sent = 0;
		bytes_received = 0;
		memset(&connection_start_time, 0, sizeof(connection_start_time));
	}

	// accessor functions
	bool           is_connected() const { return connected; }
	ip_address     get_local_ip_address() const { return local_ip_address; }
	ip_port        get_local_ip_port() const { return ip_port(local_ip_address, local_port); }
	ip_address     get_remote_ip_address() const { return remote_ip_address; }
	ip_port        get_remote_ip_port() const { return ip_port(remote_ip_address, remote_port); }
	uint16_t       get_local_port() const { return local_port; }
	uint16_t       get_remote_port() const { return remote_port; }
	uint64_t       get_bytes_sent() const { return bytes_sent; }
	uint64_t       get_bytes_received() const { return bytes_received; }
	struct timeval get_connection_start_time() const { return connection_start_time; }

private:

	// data pertaining to this connection
	bool           connected;
	ip_address     local_ip_address;
	ip_address     remote_ip_address;
	uint16_t       local_port;
	uint16_t       remote_port;
	uint64_t       bytes_sent;
	uint64_t       bytes_received;
	struct timeval connection_start_time;	// do the math on your own to calculate
																			  // actual time elapsed
	friend class tcp;
};

// metadata tcp information pertaining to the entire process
class tcp_global_info {
public:

	// default constructor
	tcp_global_info():connections_active(0),
		total_bytes_sent(0), total_bytes_received(0) {}

	// copy constructor
	tcp_global_info(const tcp_global_info& other);

	// accessor functions
	uint64_t         get_total_bytes_sent() const { return total_bytes_sent; }
	uint64_t         get_total_bytes_received() const { return total_bytes_received; }
	uint32_t         get_connections_active() const { return connections_active; }
	vector<uint16_t> get_listening_ports() const;

private:

	// metadata
	atomic<uint32_t> connections_active;
	map<uint16_t, bool> listening_ports;

	atomic<uint64_t> total_bytes_sent;
	atomic<uint64_t> total_bytes_received;

	friend class tcp;
};


// definition of the class itself
class tcp {
public:

	// constructor and destructor
	tcp();
	~tcp();

	// open, close and listen methods
	bool        connect(const ip_port& address_and_port);
	bool        connect(const string& address, uint16_t port);
	void        close();
	static bool listen(uint16_t port);
	static void stop_listen(uint16_t port);
	bool        accept(uint16_t port);

	// associate socket with a name (not required)
	void        set_name(const string& name);
	string      get_name() const;

	// methods to send data
	bool        send_raw(const autobuf& payload);
	bool        send_raw(const void* payload, int payload_len);

	// methods to receive data
	// timeout_ms: -1  means no timeout (blocking)
	//             0   means non blocking (returns immediately)
	//             +ve means wait for specified number of milliseconds before failing
	// a timeout failure returns false. check the tcp_info to find out if the connection failed or not.
	bool        recv_raw(autobuf& payload, int timeout_ms=-1);	// reads whatever was in the socket
	bool        recv_raw(void* buf, int* buf_used, int buf_len, int timeout_ms=-1);

	// reads an exact number of bytes, or fails
	bool        recv_fixed_bytes(autobuf& buf, int bytes_to_read, int timeout_ms=-1);
	bool        recv_fixed_bytes(void* buf, int bytes_to_read, int timeout_ms=-1);

	// reads until a 'token' is received. the token is included in the result
	bool        recv_until_token(autobuf& buf, const autobuf& token, int timeout_ms=-1);
	bool        recv_until_token(autobuf& buf, const void* token, int token_len, int timeout_ms=-1);

	// for advanced use -- directly gets the socket file descriptor
	int         get_fd() const;

	// get information about this current connection
	tcp_info    get_connection_info() const;

	// gets global information about the tcp state
	static tcp_global_info get_global_info();

private:

	// disable copy constructor and assignment operator
	tcp(const tcp& original);
	tcp& operator=(const tcp& original);

	// per connection information
	int socket_id;
	string name;
	tcp_info connection_info;

	// aggregate information
	static mutex lock;		// lock is only used for synchronizing listening ports
                        // stats are synchronized using c++11 atomic constructs
	static tcp_global_info global_info;
	static map<uint16_t, int> listening_port_to_socket_mapping;

	// used to perform timeout checks
	bool timeout_check(int timeout_ms);
};

// used in the construction of nonblocking tcp sockets

class epoll_set {
public:

	// constructor and destructor
	epoll_set();
	~epoll_set();

	// clears the set
	void clear();

	// used to add or remove connections to poll for
	void add(const shared_ptr<tcp>& connection);
	void remove(const shared_ptr<tcp>& connection);

	// poll function returns the set of sockets that are available for work
	// events include error, read, write 
	set<shared_ptr<tcp>> wait(int timeout_ms=-1);

private:

	int fd;
	mutex lock;
	set<shared_ptr<tcp>> watch_set;

	// used to create a new fd for the epoll set
	void init();
};
