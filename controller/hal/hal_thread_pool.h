#pragma once

#include <mutex>
#include <thread>
using namespace std;

class hal_thread_pool {
public:

	// constructor and destructor
	hal_thread_pool();
	~hal_thread_pool();

	// bind a hal controller to this thread pool
	void join_thread_pool(const shared_ptr<hal>& controller);

	// unified send queue
	void enqueue_transaction(const shared_ptr<hal>& controller, const shared_ptr<hal_transaction>& transaction);





	// used to create or locate the right thread pool
	static shared_ptr<hal_thread_pool> get_thread_pool(uint32_t thread_pool_id);
	static shared_ptr<hal_thread_pool> create_thread_pool(uint32_t thread_pool_id);


private:

	uint32_t  thread_pool_id;
	epoll_set socket_set;

	thread send_loop_thread;
	thread recv_loop_thread;
	thread process_loop_thread;

	// tcp socket listings
	shared_ptr<tcp, 


	rwqueue<pair<weak_ptr<hal>, shared_ptr<hal_transaction>>> pending_transactions;
	rwqueue<pair<weak_ptr<hal>, shared_ptr<of_message>>> asynchronous_messages;

	// maps from socket instance to a map that goes from xid to transactions
	mutex callback_lock;
	map<shared_ptr<tcp>, map<uint32_t, shared_ptr<hal_transaction>>> callback_transactions;

	// thread entrypoints
	void send_loop_entrypoint();
	void recv_loop_entrypoint();
	void process_loop_entrypoint();

	// static objects
	static mutex thread_pool_lock;
	static map<uint32_t, shared_ptr<hal_thread_pool>> thread_pool_mappings;

};
