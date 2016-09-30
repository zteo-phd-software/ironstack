#include "stacktrace.h"

char stacktrace::executable_name[1024] = { '\0' };

// signal handler to do stack traces
void stacktrace::signal_handler(int sig, siginfo_t* info, void* secret) {

	switch (sig) {
		case SIGINT:
			printf("\n*** SIGINT detected ***\n");
//			exit(0);
			break;

		case SIGABRT:
			printf("\n*** SIGABRT detected ***\n");
			break;

		case SIGSEGV:
			printf("\n*** SIGSEGV detected ***\n");
			break;

		default:
			printf("\n*** unknown signal detected ***\n");
	}

	// display stack trace
	ucontext_t *uc = (ucontext_t*) secret;
	#if __x86_64__ || __ppc64__
	printf("faulting address is %p from %p\n", info->si_addr, (void*) uc->uc_mcontext.gregs[REG_RIP]);
	#else
	printf("faulting address is %p from %p\n", info->si_addr, (void*) uc->uc_mcontext.gregs[REG_EIP]);
	#endif
	printf("stack trace is as follows:\n");

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
		printf("#%2d: function %s\n"
			"  called from ", counter, messages[counter]);

		char syscom[256];
		sprintf(syscom, "addr2line %p -e %s", trace[counter], executable_name);
		system(syscom);
	}

	// terminate
	exit(1);
}

// enables stack traces
void stacktrace::enable() {
	
	// get executable name
	char link[1024];
	sprintf(link, "/proc/%d/exe", getpid());
	if (!readlink(link, executable_name, sizeof(link)) == -1) {
		printf("stacktrace::enable() error -- cannot get executable name.\n");
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
