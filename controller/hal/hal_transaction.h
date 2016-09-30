#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include "../../common/autobuf.h"
#include "../openflow_messages/of_message.h"

class hal_callbacks;
using namespace std;

// defines a class for hal requests
//
// HOW TO USE HAL TRANSACTIONS
//
// 1. synthesize a shared_ptr to the raw openflow message you want to send
//    to the switch and populate the message fields. if a new xid is needed,
//    instantiate hal_transaction with the shortcut constructor or use the
//    assign_and_serialize() function. otherwise, if you want to keep the
//    existing xid of the message, use set_request_and_serialize().
// 2. enqueue to controller, by doing controller->enqueue_transaction(
//    shared_ptr<hal_transaction>(new hal_transaction( xxx, ... ));

class hal_transaction {
public:

	// constructors and destructor
	hal_transaction():needs_completion_acknowledgement(false) {}
	hal_transaction(const shared_ptr<of_message>& request,
		bool needs_acknowledgement,
		const shared_ptr<hal_callbacks>& cob=nullptr) {
		assign_and_serialize(request, needs_acknowledgement, cob);
	}
	~hal_transaction() {}

	// shortcut function to perform transaction id assignment and serialization
	// in one step. must also specify if a completion callback is needed
	void assign_and_serialize(const shared_ptr<of_message>& request,
		bool needs_completion_acknowledgement,
		const shared_ptr<hal_callbacks>& callback=nullptr);

	// sets the request and serializes the message. does not assign a new xid
	// (useful for replies)
	void set_request_and_serialize(const shared_ptr<of_message>& request,
		bool needs_completion_acknowledgement,
		const shared_ptr<hal_callbacks>& callback=nullptr);

	// gets the original request
	shared_ptr<of_message> get_request() const;

	// gets the serialized message
	shared_ptr<autobuf> get_serialized_msg() const;

	// gets the reply message
	shared_ptr<of_message> get_reply() const;

	// gets the setting of the acknowledgement flag
	bool has_completion_acknowledgement() const;

	// gets the callback object
	shared_ptr<hal_callbacks> get_callback() const;

	// sets the reply message
	void set_reply(const shared_ptr<of_message>& reply);

	// manually reserve one transaction value
	static uint32_t reserve_xid();

private:

	// points to the request message and the serialized content
	shared_ptr<of_message>    request;
	shared_ptr<autobuf>       serialized_msg;

	// this points to the result message upon completion (if any)
	// which could be an error message or an informative message
	shared_ptr<of_message>    reply;

	// set this flag if an acknowledgement is needed (so the controller
	// will construct a barrier for the callback)
	bool                      needs_completion_acknowledgement;

	// set this to point to a hal callback, which will be invoked on
	// error or possibly upon completion (mandatory upon completion if
	// the needs_completion_acknowledgement flag is set).
	shared_ptr<hal_callbacks> callback;

private:

	static atomic<uint32_t> xid_counter;

};

// defines a class for callbacks from the controller
class hal_callbacks {
public:

	// holds a pointer to the original request, optionally the reply
	// and the status (true = success, false = failed)
	virtual void hal_callback(const shared_ptr<hal_transaction>& transaction,
		const shared_ptr<of_message>& reply,
		bool status)=0;

};

// defines a blocking callback
// note: do not store the transaction shared pointer internally or there will
// be a reference loop and a memory leak! if you want to cache the pointer
// to the transaction, use a weak pointer.
class blocking_hal_callback : public hal_callbacks {
public:

	blocking_hal_callback():done(false),successful(false) {}

	// blocking wait functions (true means transaction completed; does not
	// mean that transaction succeeded)
	bool wait() const;
	bool timed_wait(uint32_t milliseconds) const;

	// nonblocking wait function (true means transaction completed; does not
	// mean that transaction succeeded)
	bool poll() const;

	// checks if the transaction completed successfully
	bool is_transaction_successful() const;

	// actual overriden callback
	virtual void hal_callback(const shared_ptr<hal_transaction>& transaction,
		const shared_ptr<of_message>& reply,
		bool status);

private:

	bool                        done;
	bool                        successful;
	mutable mutex               lock;
	mutable condition_variable  cond;
};

