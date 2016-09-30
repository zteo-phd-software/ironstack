#ifndef __AUX_TCP_UTILITIES
#define __AUX_TCP__UTILITIES

#include <string>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "hash.h"
#include "tcp.h"

// metadata object that holds the information on existing file transfers
class __transfer_data
{
public:
	__transfer_data();

	std::string filename;
	uint32_t file_hash;

	bool error;
	bool completed;
	
	long bytes_transferred;
	long bytes_total;
	float percentage_complete;

	float transfer_rate;				// kilobytes per second
	float transfer_time;				// amount of time since transfer started (in seconds)
	float estimated_time_remaining;		// estimated time left in seconds
};

// object that is returned when performing a background file transfer
class transfer_progress
{
public:
	transfer_progress();
	~transfer_progress();

	__transfer_data get_data_async();
	__transfer_data get_data_blocked();

	bool completed;

	// do not touch the rest of these fields!
	pthread_mutex_t lock;
	pthread_cond_t cond;

	struct timeval start_time;
	__transfer_data status;
};

namespace tcp_utils
{

	// transfers or resumes a file, backed by hash function
	// blocks entirely while waiting for file transfer to complete, returns -1 on error, or 0 on success.
	int transfer_file_blocked(tcp* connection, const std::string& filename);

	// transfers or resumes a file, backed by hash function
	// runs in async mode, updates are available by probing
	transfer_progress* transfer_file_async(tcp* connection, const std::string& filename);

	// thread entrypoint to transfer a file
	static void* transfer_file_async_entrypoint(void* args);

}

#endif
