#include <string.h>
#include "output.h"

weak_ptr<gui_controller> output::controller;
atomic_bool output::log_printf{false};
output::loglevel output::loglevel_threshold{output::loglevel::WARNING};
mutex output::fp_lock;
FILE* output::fp = nullptr;

// sets up output to point to a controller
void output::init(const shared_ptr<gui_controller>& controller_) {
	controller = controller_;
}

// shuts down the gui controller, if any
void output::shutdown() {
	shared_ptr<gui_controller> current_controller = controller.lock();

	if (current_controller != nullptr) {
		current_controller->shutdown();
		current_controller = nullptr;
		controller = current_controller;
	}
}

// attempts to start logging output to file
bool output::start_log(const string& filename, bool log_printf_, loglevel loglevel_display_threshold) {
	log_printf = log_printf_;
	
	lock_guard<mutex> g(fp_lock);
	if (fp != nullptr) {
		fclose(fp);
	}

	fp = fopen(filename.c_str(), "a");
	if (fp != nullptr) {
		char timestamp[128];
		get_timestamp(timestamp);
		fprintf(fp, "### LOG STARTED [%s] ###\n", timestamp);
	} else {
		log_printf = false;
	}

	loglevel_threshold = loglevel_display_threshold;

	return fp == nullptr;
}

// stops logging to file
void output::stop_log() {
	lock_guard<mutex> g(fp_lock);
	if (fp != nullptr) {
		fclose(fp);
	}
	fp = nullptr;
	log_printf = false;
}

// prints to either stdout or through the GUI controller
void output::printf(const char* fmt, ...) {
	char buf[10240];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, args);

	shared_ptr<gui_controller> current_controller = controller.lock();
	if (current_controller != nullptr) {
		current_controller->printf("%s", buf);
	} else {
		::printf("%s", buf);
	}

	// log to file if required
	if (log_printf) {
		log("%s", buf);
	}
}

// logs directly to file. does not appear on display. no loglevel associated.
void output::log(const char* fmt, ...) {
	char buf[10240];
	char timestamp[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf)-33, fmt, args);

	// get timestamp
	get_timestamp(timestamp);

	// write to file
	lock_guard<mutex> g(fp_lock);
	if (fp != nullptr) {
		fprintf(fp, "[%s] %s", timestamp, buf);

		// append a newline if it wasn't given
		int len = strlen(buf);
		if (len == 0 || buf[len-1] != '\n') {
			fprintf(fp, "\n");
		}
	}
}

// logs directly to file, with loglevel
void output::log(const loglevel& level, const char* fmt, ...) {

	char buf[10240];
	char timestamp[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf)-33, fmt, args);

	// get timestamp
	get_timestamp(timestamp);

	// generate output string
	char result[1024+128+2];
	switch (level) {
		case loglevel::VERBOSE:
		{
			snprintf(result, sizeof(result), "[%s] VERBOSE: %s", timestamp, buf);
			break;
		}
		case loglevel::INFO:
		{
			snprintf(result, sizeof(result), "[%s] INFO: %s", timestamp, buf);
			break;
		}
		case loglevel::WARNING:
		{
			snprintf(result, sizeof(result), "[%s] WARNING: %s", timestamp, buf);
			break;
		}
		case loglevel::CRITICAL:
		{
			snprintf(result, sizeof(result), "[%s] CRITICAL: %s", timestamp, buf);
			break;
		}
		case loglevel::ERROR:
		{
			snprintf(result, sizeof(result), "[%s] ERROR: %s", timestamp, buf);
			break;
		}
		case loglevel::BUG:
		{
			snprintf(result, sizeof(result), "[%s] BUG: %s", timestamp, buf);
			break;
		}
	}

	// check if newline was written. if not, add one.
	int len = strlen(buf);
	if (len == 0 || buf[len-1] != '\n') {
		strcat(buf, "\n");
	}

	// write to file
	{
		lock_guard<mutex> g(fp_lock);
		if (fp != nullptr) {
			fprintf(fp, "%s", result);
		}
	}

	// display to output as required
	int loglevel_threshold_num = (int) loglevel_threshold;
	int loglevel_current_num = (int) level;

	if (loglevel_current_num >= loglevel_threshold_num) {
		shared_ptr<gui_controller> current_controller = controller.lock();
		if (current_controller != nullptr) {
			current_controller->printf("%s", result);
		} else {
			::printf("%s", result);
		}
	}
}

// generates a timestamp
void output::get_timestamp(char* timestamp) {
	time_t rawtime;
	struct tm* info;

	time(&rawtime);
	info = localtime(&rawtime);
	strftime(timestamp, 128, "%d/%b %H:%M:%S", info);
}

