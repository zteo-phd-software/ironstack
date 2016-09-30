#include "key_reader.h"
using namespace gui;

// constructor
key_reader::key_reader() {
	set_input_sequence(-1000);
}

// destructor
key_reader::~key_reader() { }

// clears all keys from the queue
void key_reader::clear() {
	lock_guard<mutex> g(lock);
	keys.clear();
}

// check if at least one key is present
bool key_reader::has_key() const {
	lock_guard<mutex> g(lock);
	return !keys.empty();
}

// get the number of keys in the buffer
uint32_t key_reader::get_pending_key_count() const {
	lock_guard<mutex> g(lock);
	return keys.size();
}

// waits for a certain amount of time for a key to be pressed
// if timeout is set to 0, it blocks. default behavior blocks.
keystroke key_reader::wait_for_key(uint32_t timeout_ms) {
	unique_lock<mutex> l(lock);
	if (timeout_ms == 0) {
		cond.wait(l);
	} else {
		cond.wait_for(l, chrono::milliseconds(timeout_ms));
	}

	if (keys.empty()) {
		return keystroke();
	} else {
		auto result = keys.front();
		keys.pop_front();
		return result;
	}
}

// non-blocking check to see if a key is in the buffer
keystroke key_reader::poll_for_key() {
	lock_guard<mutex> g(lock);
	if (keys.empty()) {
		return keystroke();
	} else {
		auto result = keys.front();
		keys.pop_front();
		return result;
	}
}

// returns a vector of all keys in the buffer
vector<keystroke> key_reader::get_all_keys() {
	lock_guard<mutex> g(lock);
	vector<keystroke> result;
	result.reserve(keys.size());
	for (const auto& key : keys) {
		result.push_back(key);
	}
	keys.clear();
	return result;
}

// returns the first available key without removing it from the buffer
keystroke key_reader::peek_key() const {
	lock_guard<mutex> g(lock);
	if (!keys.empty()) {
		return keys.front();
	} else {
		return keystroke();
	}
}

// returns all available keys without removing them from the buffer
vector<keystroke> key_reader::peek_all_keys() const {
	lock_guard<mutex> g(lock);
	vector<keystroke> result;

	result.reserve(keys.size());
	for (const auto& key : keys) {
		result.push_back(key);
	}
	return result;
}

// function to receive keystrokes from the controller
void key_reader::process_keypress(const keystroke& key) {
	unique_lock<mutex> l(lock);
	keys.push_back(key);
	cond.notify_all();
}
