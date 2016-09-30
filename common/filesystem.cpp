#include "filesystem.h"
using namespace std;

// generates a list of all files within this directory (pathname omitted in filenames)
// inputs:	string containing the directory to list files. if no directory is supplied, the current directory is implied.
// 			a list reference that will be populated with the list results
// returns: 0 if successful
//			-1 if directory name isn't found, or if listing failed for some reason
int filesystem::list_files(const string& directory, std::list<string>& file_list)
{
	file_list.clear();

	// decide which directory to open
	DIR* dirp = opendir(directory.size() == 0 ? filesystem::get_working_directory().c_str() : directory.c_str());
	if (dirp == NULL)
		return -1;

	// directory open, now list the files
	struct dirent* result = NULL;
	struct dirent entry;
	int return_value;
	for (return_value = readdir_r(dirp, &entry, &result); result != NULL && return_value == 0; return_value = readdir_r(dirp, &entry, &result))
	{
		if (is_file(string(entry.d_name).insert(0, directory)))
			file_list.push_back(entry.d_name);
	}
	closedir(dirp);
	return 0;
}

// generates a list of all directories within this directory (pathname omitted in filenames)
// . and .. are not listed
int filesystem::list_directories(const string& directory, std::list<string>& dir_list)
{
	dir_list.clear();

	// decide which directory to open
	DIR* dirp = opendir(directory.size() == 0 ? filesystem::get_working_directory().c_str() : directory.c_str());
	if (dirp == NULL)
		return -1;

	// directory open, now list the files
	struct dirent* result = NULL;
	struct dirent entry;
	int return_value;
	for (return_value = readdir_r(dirp, &entry, &result); result != NULL && return_value == 0; return_value = readdir_r(dirp, &entry, &result))
	{
		if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0)
			continue;

		if (is_directory(string(entry.d_name).insert(0, directory)))
			dir_list.push_back(entry.d_name);
	}
	closedir(dirp);
	return 0;
	
}

// recursively generates a list of all files (generates full pathnames because it's recursive)
int filesystem::recursive_list_files(const string& src_directory, std::list<string>& file_list)
{
	file_list.clear();
	string selected_directory = (src_directory.size() == 0 ? filesystem::get_working_directory() : src_directory);
	if (selected_directory.c_str()[selected_directory.size()-1] != '/')
		selected_directory.append("/");

	// first add the root directory contents, and prepend the full pathname
	if (list_files(selected_directory, file_list) < 0)
		return -1;
	for(std::list<string>::iterator iterator = file_list.begin(); iterator != file_list.end(); iterator++)
		iterator->insert(0, selected_directory);
	
	// enumerate the directories and recusively iterate over them
	std::list<string> directories;
	if (list_directories(selected_directory, directories) < 0)
		return -1;

	std::list<string> dir_file_list;
	for (std::list<string>::iterator iterator = directories.begin(); iterator != directories.end(); iterator++)
	{
		iterator->insert(0, selected_directory);
		if (recursive_list_files(*iterator, dir_file_list) < 0)
			return -1;
		file_list.insert(file_list.end(), dir_file_list.begin(), dir_file_list.end());
	}
	
	return 0;
}

// switches to a working directory, -1 means failure, 0 means OK
int filesystem::change_working_directory(const string& target_directory)
{
	return chdir(target_directory.c_str());
}

// returns the current working directory
string filesystem::get_working_directory()
{
	char buf[MAX_PATH_LEN];
	if (getcwd(buf, sizeof(buf)) == NULL)
		return string("");
	else
		return string(buf);
}

// returns the size of a file, -1 if file doesn't exist or is a directory
long filesystem::get_file_size(const string& filename)
{
	struct stat statbuf;
	if (stat(filename.c_str(), &statbuf) < 0)
		return -1;
	else
		return statbuf.st_size;
}

// returns true if the supplied pathname is a directory
bool filesystem::is_directory(const string& pathname_to_test)
{
	struct stat statbuf;
	if (stat(pathname_to_test.c_str(), &statbuf) == -1)
		return false;
	
	if (S_ISDIR(statbuf.st_mode))
		return true;
	else
		return false;
}

// returns true if the supplied filename is a regular file
bool filesystem::is_file(const string& filename_to_test)
{
	struct stat statbuf;
	if (stat(filename_to_test.c_str(), &statbuf) == -1)
		return false;

	if (S_ISREG(statbuf.st_mode))
		return true;
	else
		return false;

}

