#include "hal_transaction.h"

// transaction counter for hal
atomic<uint32_t> hal_transaction::xid_counter(1);

// assigns a request, gives it a transaction ID and serializes the request
void hal_transaction::assign_and_serialize(const shared_ptr<of_message>& request_,
	bool needs_acknowledgement,
	const shared_ptr<hal_callbacks>& cob) {
	
	// get a new transaction id
	request = request_;
	request->xid = reserve_xid();
	
	// serialize the message
	serialized_msg.reset(new autobuf());
	request->serialize(*serialized_msg.get());

	// setup other parameters
	needs_completion_acknowledgement = needs_acknowledgement;
	callback = cob;
}

// sets up a request (without chainging the xid), and serializes the request
void hal_transaction::set_request_and_serialize(const shared_ptr<of_message>& request_,
	bool needs_acknowledgement,
	const shared_ptr<hal_callbacks>& cob) {

	request = request_;
	serialized_msg.reset(new autobuf());
	request->serialize(*serialized_msg.get());
	needs_completion_acknowledgement = needs_acknowledgement;
	callback = cob;
}

// returns the original request message
shared_ptr<of_message> hal_transaction::get_request() const {
	return request;
}

// returns the serialized autobuf msg
shared_ptr<autobuf> hal_transaction::get_serialized_msg() const {
	return serialized_msg;
}

// gets the reply message
shared_ptr<of_message> hal_transaction::get_reply() const {
	return reply;
}

// checks the status of the acknowledgement flag
bool hal_transaction::has_completion_acknowledgement() const {
	return needs_completion_acknowledgement;
}

// gets the callback object
shared_ptr<hal_callbacks> hal_transaction::get_callback() const {
	return callback;
}

// sets the transaction reply
void hal_transaction::set_reply(const shared_ptr<of_message>& reply_) {
	reply = reply_;
}

// assigns a new transaction number atomically
uint32_t hal_transaction::reserve_xid() {
	return xid_counter++;
}

// unblocks any potential waiting callers
void blocking_hal_callback::hal_callback(const shared_ptr<hal_transaction>& transaction_reply,
	const shared_ptr<of_message>& openflow_reply,
	bool status) {

	unique_lock<mutex> g(lock, defer_lock);
	g.lock();
	done = true;
	if (openflow_reply != nullptr && openflow_reply->msg_type == OFPT_ERROR) {
		successful = false;
	} else {
		successful = true;
	}

	cond.notify_all();
	g.unlock();
}

// blocks while waiting to hear back from the controller
bool blocking_hal_callback::wait() const {
	
	unique_lock<mutex> g(lock, defer_lock);
	g.lock();
	while (!done) {
		cond.wait(g);
	}
	g.unlock();

	return true;
}

// blocks up to a certain interval waiting to hear back from the controller
bool blocking_hal_callback::timed_wait(uint32_t milliseconds) const {

	bool result = true;
	unique_lock<mutex> g(lock, defer_lock);
	g.lock();
	if (!done && cond.wait_for(g, chrono::milliseconds(milliseconds)) == cv_status::timeout) {
		result = false;
	}
	g.unlock();

	return result;
}

// polls to check if the request completed
bool blocking_hal_callback::poll() const {

	bool result;
	unique_lock<mutex> g(lock, defer_lock);
	g.lock();
	result = done;
	g.unlock();

	return result;
}

// checks if the transaction completed and was successful
bool blocking_hal_callback::is_transaction_successful() const {
	return successful;
}
