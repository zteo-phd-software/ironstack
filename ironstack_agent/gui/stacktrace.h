#pragma once
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <execinfo.h>
#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// function prototype

// to turn on stack traces, simply run stacktrace::enable().
// stack trace will fire on SIGABRT, SIGINT and SIGSEGV.
class stacktrace {
public:
	static void enable();

private:

	static void signal_handler(int sig, siginfo_t* info, void* secret);
	static char executable_name[1024];
};
