#pragma once
#include <condition_variable>
#include <list>
#include <vector>
#include "gui_component.h"
using namespace std;

namespace gui {

/*
 * this is a keyboard reader class, which has no visible element.
 * useful for waiting on or monitoring keystrokes.
 *
 * how to use:
 * 1. register the key_reader as a GUI input component. do not register
 *    this as a displayable component.
 * 2. call one of the key grabbing functions.
 *
 * the reader component has a very high priority for input sequencing
 * and will preferentially receive input over other components. to change
 * this priority, call set_input_sequence()
 */

class key_reader : public input_component {
public:

	// constructor and destructor
	key_reader();
	virtual ~key_reader();

	// removes all keys from the pending queue
	void              clear();

	// check for keys
	bool              has_key() const;
	uint32_t          get_pending_key_count() const;
	keystroke         wait_for_key(uint32_t timeout_ms=0); // default: 0=blocking
	keystroke         poll_for_key(); // returns immediately
	vector<keystroke> get_all_keys();

	// these functions allow you to look at keystrokes without removing them
	// from the buffer
	keystroke         peek_key() const;
	vector<keystroke> peek_all_keys() const;

	// required functions from the input component class
	virtual void      process_keypress(const keystroke& key);
	virtual void      on_focus() {}
	virtual void      on_blur() {}

private:

	// disable copies
	key_reader(const key_reader& other);
	key_reader& operator=(const key_reader& other);

	// state
	mutable mutex              lock;
	mutable condition_variable cond;
	list<keystroke>            keys;
};
};
