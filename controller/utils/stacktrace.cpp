#include "stacktrace.h"
#include "../gui/output.h"
#include <string.h>
#include <unistd.h>
char stacktrace::executable_name[1024] = { '\0' };

// signal handler to do stack traces
void stacktrace::signal_handler(int sig, siginfo_t* info, void* secret) {
	static bool triggered = false;
	if (triggered) return;
	triggered = true;

	output::shutdown();

	switch (sig) {
		case SIGINT:
			output::log(output::loglevel::INFO, "\n*** SIGINT detected ***\n");
			exit(0);
			break;

		case SIGABRT:
			output::log(output::loglevel::INFO, "\n*** SIGABRT detected ***\n");
			break;

		case SIGSEGV:
			output::log(output::loglevel::BUG, "\n*** SIGSEGV detected ***\n");
			break;

		default:
			output::log(output::loglevel::INFO, "\n*** unknown signal detected ***\n");
	}

	// display stack trace
	ucontext_t *uc = (ucontext_t*) secret;
	#if __x86_64__ || __ppc64__
	output::log(output::loglevel::INFO, "faulting address is %p from %p\n", info->si_addr,
		(void*) uc->uc_mcontext.gregs[REG_RIP]);
	#else
	output::log(output::loglevel::INFO, "faulting address is %p from %p\n", info->si_addr,
		(void*) uc->uc_mcontext.gregs[REG_EIP]);
	#endif
	output::log(output::loglevel::INFO, "stack trace is as follows:\n");

	void* trace[16];
	char** messages = (char**) nullptr;
	int counter, trace_size = 0;
	trace_size = backtrace(trace, 16);
	#if __x86_64__ || __ppc64__
	trace[1] = (void*) uc->uc_mcontext.gregs[REG_RIP];
	#else
	trace[1] = (void*) uc->uc_mcontext.gregs[REG_EIP];
	#endif

	messages = backtrace_symbols(trace, trace_size);
	for (counter = 1; counter < trace_size; ++counter) {
		output::log(output::loglevel::INFO, "#%2d: function %s "
			"called from \n     ", counter, messages[counter]);

		// run command to get address
		char syscom[256];
		sprintf(syscom, "/usr/bin/addr2line %p -e %s > addr.txt\n", trace[counter],
			executable_name);
		system(syscom);

		// read in the address
		char buf[128] = {0};
		FILE* fp = fopen("addr.txt", "r");
		if (fp != nullptr) {
			fgets(buf, sizeof(buf)-1, fp);
			fclose(fp);
		}
		for (int counter2 = 0; counter2 < (int) sizeof(buf); ++counter2) {
			if (buf[counter2] == '\n') buf[counter2] = 0;
		}

		// replace useless information with something more informative
		if (strcmp(buf, "??:0") == 0 || strcmp(buf, "??:?") == 0) {
			sprintf(buf, "<unknown address>");
		}

		output::log(output::loglevel::INFO, "%s\n", buf);
		unlink("addr.txt");
		
	}

	// terminate
	exit(1);
}

// enables stack traces
void stacktrace::enable() {
	
	// get executable name
	char link[1024];
	sprintf(link, "/proc/%d/exe", getpid());
	if (readlink(link, executable_name, sizeof(link)) == -1) {
		output::log(output::loglevel::ERROR, "stacktrace::enable() cannot get executable name.\n");
		exit(1);
	}

	// setup signal handler
	struct sigaction sa;
	sa.sa_sigaction = signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGSEGV, &sa, nullptr);

}
