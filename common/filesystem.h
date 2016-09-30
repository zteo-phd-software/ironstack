#ifndef __FILESYSTEM_UTILITIES
#define __FILESYSTEM_UTILITIES

#include <list>
#include <string>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

// thread safety: all functions are thread safe.

#define MAX_PATH_LEN 2048

namespace filesystem
{
	// generates a list of all files within this directory (pathname omitted in filenames)
	// input: directory -- if this is specified to be "", will assume current directory, otherwise it assumes relative path
	// returns 0 on success, -1 on failure
	int			list_files(const std::string& directory, std::list<std::string>& file_list);

	// generates a list of all directories within this directory (pathname omitted in filenames); . and .. entries are omitted
	// input: directory -- if this is specified to be "", will assume current directory, otherwise it assumes relative path
	// returns 0 on success, -1 on failure
	int			list_directories(const std::string& directory, std::list<std::string>& dir_list);

	// recursively generates a list of all files (generates full pathnames because it's recursive)
	// input: directory -- if this is specified to be "", will assume current directory, otherwise it assumes relative path
	// returns 0 on success, -1 on failure
	// all returned filenames are regular files and will contain full pathnames
	int			recursive_list_files(const std::string& src_directory, std::list<std::string>& file_list_full_pathname);

	// switches to a working directory
	// returns 0 on success, -1 on failure
	int			change_working_directory(const std::string& target_directory);

	// returns the current working directory
	// returns "" if there was an error getting the cwd (rare, but possible if the pathname is too long)
	std::string	get_working_directory();

	// returns the size of a file, or -1 if file doesn't exist or is a directory
	long		get_file_size(const std::string& filename);

	// returns true if the supplied pathname is a directory
	bool		is_directory(const std::string& pathname_to_test);

	// returns true if the supplied filename is a regular file
	bool		is_file(const std::string& filename_to_test);
};

#endif
