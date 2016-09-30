#ifndef __CONSOLE_COLORS
#define __CONSOLE_COLORS

// foreground color definitions
#define BLACK			"\033[0;30m"
#define BLUE			"\033[0;34m"
#define GREEN			"\033[0;32m"
#define CYAN			"\033[0;36m"
#define RED				"\033[0;31m"
#define MAGENTA			"\033[0;35m"
#define BROWN			"\033[0;33m"
#define GRAY			"\033[0;37m"
#define DARK_GRAY		"\033[1;30m"
#define LIGHT_BLUE		"\033[1;34m"
#define LIGHT_GREEN		"\033[1;32m"
#define LIGHT_CYAN		"\033[1;36m"
#define LIGHT_RED		"\033[1;31m"
#define LIGHT_MAGENTA	"\033[1;35m"
#define YELLOW			"\033[1;33m"
#define WHITE			"\033[1;37m"

// background color definitions
#define BG_BLACK		"\033[40m"
#define BG_BLUE			"\033[44m"
#define BG_GREEN		"\033[42m"
#define BG_CYAN			"\033[46m"
#define BG_RED			"\033[41m"
#define BG_MAGENTA		"\033[45m"
#define BG_BROWN		"\033[43m"
#define BG_GRAY			"\033[47m"

// use at the end to restore printing to original attributes
#define RESTORE			"\033[40m\033[1;37m"

#endif
