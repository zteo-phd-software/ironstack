// used for supressing debug information
#pragma once

#ifdef __REDIRECT_PRINTF_TO_STDERR
#define printf(...) \
	do { fprintf(stderr, __VA_ARGS__); } while(0)
#endif

