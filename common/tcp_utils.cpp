#include "tcp_utils.h"

// constructor to initialize inspector object
__transfer_data::__transfer_data()
{
	filename = "";
	file_hash = 0;
	error = true;
	completed = false;
	bytes_transferred = 0;
	bytes_total = 0;
	percentage_complete = 0;
	transfer_rate = 0;
	transfer_time = 0;
	estimated_time_remaining = 0;
}

// constructor to initialize synchronization stuff
transfer_progress::transfer_progress()
{
	completed = false;
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&cond, NULL);
}

// destructor removes synchronization stuff
transfer_progress::~transfer_progress()
{
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&cond);
}

// used to get async updates about the file transfer
__transfer_data transfer_progress::get_data_async()
{
	__transfer_data result;

	if (completed)
		return status;

	pthread_mutex_lock(&lock);
	result = status;
	pthread_mutex_unlock(&lock);

	return result;
}

// blocks until the next update is available
__transfer_data transfer_progress::get_data_blocked()
{
	__transfer_data result;

	pthread_mutex_lock(&lock);
	if (completed)
		result = status;
	else
	{
		pthread_cond_wait(&cond, &lock);
		result = status;
	}
	pthread_mutex_unlock(&lock);

	return result;
}

// performs a file transfer, blocking until either the file completes or an error occurs
int tcp_utils::transfer_file_blocked(tcp* connection, const std::string& filename)
{
	long file_size = filesystem::get_file_size(filename);
	uint32_t file_hash = hash::get_generic_hash_from_file(filename);
	long bytes_processed = 0;
	long resume_offset = 0;
	int chunk_size;
	char buf[2048];
	std::string response;

	if (file_size < 0 || file_hash == 0)
		return -2;

	FILE* fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
		return -2;
	
	// tell the remote file name, hash and filesize
	sprintf(buf, "%ld", file_size);
	if (connection->write_string(filename.c_str()) < 0
		|| connection->write_uint32(file_hash) < 0
		|| connection->write_string(buf) < 0)
		goto fail;

	// wait for remote to respond
	if (connection->read_string(response) < 0)
		goto fail;

	// check if starting from the beginning or resuming
	if (response.compare("OK") == 0)
	{}
	else if (response.compare("CONTINUE") == 0)
	{
		// read in resume value
		if (connection->read_string(response) < 0
			|| sscanf(response.c_str(), "%ld", &resume_offset) != 1)
			goto fail;

		// scroll to the point in the file
		if (fseek(fp, resume_offset, SEEK_SET) < 0)
			goto fail;

		bytes_processed = resume_offset;
	}
	else
		goto fail;


	// read in file blocks, 2k at a time if possible
	while (bytes_processed < file_size)
	{
		chunk_size = (int) (file_size - bytes_processed >= (int) sizeof(buf) ? sizeof(buf) : file_size - bytes_processed);
		if (feof(fp) || fread(buf, 1, chunk_size, fp) < (size_t) chunk_size)
		{
			// file contents changed! wtf.
			goto fail;
		}

		// transmit to remote
		if (connection->write_data_item(buf, sizeof(buf)) < 0)
			goto fail;
	}

	// everything sent
	return 0;
	
fail:
	connection->write_string("ERROR");
	fclose(fp);
	return -1;
}

// performs an async file transfer in the background, updating as the file transfer proceeds
transfer_progress* tcp_utils::transfer_file_async(tcp* connection, const std::string& filename)
{
	pthread_t tid;

	// setup transfer object
	transfer_progress* progress = new transfer_progress();
	progress->status.filename = filename;
	progress->status.file_hash = hash::get_generic_hash_from_file(filename);
	progress->status.error = false;
	progress->status.completed = false;
	progress->status.bytes_transferred = 0;
	progress->status.bytes_total = filesystem::get_file_size(filename);
	progress->status.percentage_complete = 0;
	progress->status.transfer_rate = 0;
	progress->status.transfer_time = 0;
	progress->status.estimated_time_remaining = -1;

	// create thread parameters
	typedef struct
	{
		transfer_progress* progress;
		tcp* connection;
	} params_t;
	params_t* params = (params_t*) malloc(sizeof(params));
	params->progress = progress;
	params->connection = connection;

	// sanity check
	if (progress->status.bytes_total < 0
		|| progress->status.file_hash == 0
		|| pthread_create(&tid, NULL, transfer_file_async_entrypoint, params) != 0)
	{
		free(params);
		delete progress;
		return NULL;
	}

	return progress;
}

// thread entrypoint to transfer the file
void* tcp_utils::transfer_file_async_entrypoint(void* args)
{
	typedef struct
	{
		transfer_progress* progress;
		tcp* connection;
	} params_t;

	// copy out parameters and free the struct
	transfer_progress* progress = ((params_t*)args)->progress;
	tcp* connection = ((params_t*)args)->connection;
	free(args);

	// detach thread
	pthread_detach(pthread_self());

	// now transfer the file
	long file_size = filesystem::get_file_size(progress->status.filename);
	uint32_t file_hash = hash::get_generic_hash_from_file(progress->status.filename);
	long bytes_processed = 0;
	long resume_offset = 0;
	int chunk_size;
	char buf[2048];
	std::string response;
	struct timeval current_time;
	float chunk_elapsed_time;
	int chunk_round;
	FILE* fp = fopen(progress->status.filename.c_str(), "r");

	// sanity checks
	if (file_size < 0 || file_hash == 0 || fp == NULL)
		goto fail;

	// tell the remote file name, hash and filesize
	sprintf(buf, "%ld", file_size);
	if (connection->write_string(progress->status.filename.c_str()) < 0
		|| connection->write_uint32(file_hash) < 0
		|| connection->write_string(buf) < 0)
		goto fail;

	// wait for remote to respond
	if (connection->read_string(response) < 0)
		goto fail;

	// check if starting from the beginning or resuming
	if (response.compare("OK") == 0)
	{}
	else if (response.compare("CONTINUE") == 0)
	{
		// read in resume value
		if (connection->read_string(response) < 0
			|| sscanf(response.c_str(), "%ld", &resume_offset) != 1)
			goto fail;

		// scroll to the point in the file
		if (fseek(fp, resume_offset, SEEK_SET) < 0)
			goto fail;

		bytes_processed = resume_offset;
	}
	else
		goto fail;

	// update progress
	gettimeofday(&current_time, NULL);
	chunk_elapsed_time = 0.0;
	chunk_round = 0;
	pthread_mutex_lock(&progress->lock);
	progress->status.bytes_transferred = bytes_processed;
	progress->status.percentage_complete = (float) bytes_processed / (float) file_size * 100.0f;
	gettimeofday(&progress->start_time, NULL);
	pthread_mutex_unlock(&progress->lock);

	// read in file blocks, 2k at a time if possible
	while (bytes_processed < file_size)
	{
		chunk_size = (int) (file_size - bytes_processed >= (int) sizeof(buf) ? sizeof(buf) : file_size - bytes_processed);
		if (feof(fp) || fread(buf, 1, chunk_size, fp) < (size_t) chunk_size)
		{
			// file contents changed! wtf.
			goto fail;
		}

		// transmit to remote
		if (connection->write_data_item(buf, sizeof(buf)) < 0)
			goto fail;

		// update progress
		pthread_mutex_lock(&progress->lock);
		progress->status.bytes_transferred += chunk_size;
		progress->status.percentage_complete = (float) bytes_processed / (float) file_size * 100.0f;
		progress->status.transfer_time = cu_get_time_elapsed_to_now(progress->start_time);

		chunk_round++;
		if (chunk_round == 5)
		{
			chunk_round = 0;
			chunk_elapsed_time = cu_get_time_elapsed_to_now(current_time);
			if (chunk_elapsed_time > 0.0)
				progress->status.transfer_rate = (4*sizeof(buf) + chunk_size) / (1024.0 * cu_get_time_elapsed_to_now(current_time));
			else
				progress->status.transfer_rate = -1;	// transfer rate too high
			gettimeofday(&current_time, NULL);
		}
		
		if (progress->status.transfer_rate > 0.0)
			progress->status.estimated_time_remaining = (file_size - progress->status.bytes_transferred) / progress->status.transfer_rate;
		else
			progress->status.estimated_time_remaining = -1;

		pthread_cond_broadcast(&progress->cond);
		pthread_mutex_unlock(&progress->lock);
	}

	// everything sent, update the transfer information one last time
	pthread_mutex_lock(&progress->lock);
	progress->status.completed = true;
	progress->status.estimated_time_remaining = 0;
	progress->status.transfer_time = cu_get_time_elapsed_to_now(progress->start_time);
	progress->status.percentage_complete = 100.0;

	pthread_cond_broadcast(&progress->cond);
	pthread_mutex_unlock(&progress->lock);

	progress->completed = true;
	return NULL;
	
fail:
	connection->write_string("ERROR");
	fclose(fp);
	progress->status.error = true;
	pthread_mutex_lock(&progress->lock);
	pthread_cond_broadcast(&progress->cond);
	pthread_mutex_unlock(&progress->lock);

	return NULL;
}
