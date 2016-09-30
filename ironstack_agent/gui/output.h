#pragma once
#include <atomic>
#include <memory>
#include <string>
#include "gui_controller.h"

/*
 * all program output should be redirected to this class.
 * output will be sent to text GUI (if applicable), or stderr.
 * optionally, output may be logged from this module.
 */
class output {
public:

	// enum type for priority classes
	enum class loglevel { VERBOSE, INFO, WARNING, CRITICAL, ERROR, BUG };

	// central on/off functionality
	static void init(const shared_ptr<gui_controller>& controller);
	static void shutdown();

	// used to control logging. log_printf controls if printfs() called through this output
	// module will be logged. log_display_threshold determines what severity of errors should
	// also be sent to the output when log() is called.
	static bool start_log(const string& filename, bool log_printf=true, loglevel log_display_threshold=loglevel::WARNING);
	static void stop_log();

	// output. printf() will send to screen and log (if set)
	static void printf(const char* fmt, ...);
	static void log(const char* fmt, ...);				// sends to logfile only -- does not display
	static void log(const loglevel& level, const char* fmt, ...); // logs to file and also displays if the display threshold is lower than the loglevel

private:
	
	static weak_ptr<gui_controller> controller;
	static atomic_bool              log_printf;
	static loglevel                 loglevel_threshold;	// threshold for display
	static mutex                    fp_lock;
	static FILE*                    fp;

	static void get_timestamp(char* timestamp);
};
