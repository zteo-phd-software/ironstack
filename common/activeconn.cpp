#include <iostream>
#include <string>
#include <set>
#include <list>
#include <fstream>
#include <sstream>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "dirent.h"

#include "ip_port.h"

using namespace std;

class socket_info {
	public:
		int socket_id;
		bool socket_is_tcp;
		bool socket_is_udp;

		//Change this to ip_port if that is what you want, just change the constuctor too
		unsigned int local_ip,remote_ip;
		unsigned short local_port,remote_port;

		socket_info(unsigned int d, unsigned int dp, unsigned int s, unsigned int sp ){
			//Should you want to use ip_port, change the variables and call the ip_port
			//constructor here
			local_ip = d;
			local_port = dp;
			remote_ip = s;
			remote_port = sp;

			socket_is_tcp = false;
			socket_is_udp = false;
		}

		~socket_info(){

		}
};


list<socket_info*>* filterConnByInode(set<unsigned int> inodes){
	ifstream file;
	string tokens;

	list<socket_info*> *result = new list<socket_info*>();
	string local,remote,inode;
	int i;

	//Check the list of TCP connections
	file.open("/proc/net/tcp");
	while(file.is_open() && file.good()){
		string line;
		getline(file,line);

		if(line.length() < 1){
			continue;
		}

		//I suck at C++ string parsing, but get the address column
		istringstream ss(line);
		for(i=0;i<2;i++){
			ss >> local;
		}
		ss >> remote;

		//Get the inode column
		for(i=4;i<11;i++){
			ss >> inode;
		}

		i = 0;
		socket_info *temp;
		unsigned int ip1;
		unsigned short port1;
		unsigned int ip2;
		unsigned short port2;

		try{
			stringstream(inode) >> i;
			if(inodes.find(i) != inodes.end()){
				stringstream(local.substr(0,8)) >> hex >> ip1;
				stringstream(local.substr(9)) >> hex >> port1;
				stringstream(remote.substr(0,8)) >> hex >> ip2;
				stringstream(remote.substr(9)) >> hex >> port2;

				//Add this unless it's a listening connection
				if(ip2 != 0){
					temp = new socket_info(ip1,port1,ip2,port2);
					temp->socket_is_tcp = true;
					result->push_back(temp);
				}
			}
		} catch (...){
		}
	}
	file.close();

	file.open("/proc/net/udp");
	while(file.is_open() && file.good()){
		string line;
		getline(file,line);

		if(line.length() < 1){
			continue;
		}

		istringstream ss(line);
		for(i=0;i<2;i++){
			ss >> local;
		}
		ss >> remote;

		for(i=4;i<11;i++){
			ss >> inode;
		}

		i = 0;
		socket_info *temp;
		unsigned int ip1;
		unsigned short port1;
		unsigned int ip2;
		unsigned short port2;

		try{
			stringstream(inode) >> i;
			if(inodes.find(i) != inodes.end()){
				stringstream(local.substr(0,8)) >> hex >> ip1;
				stringstream(local.substr(9)) >> hex >> port1;
				stringstream(remote.substr(0,8)) >> hex >> ip2;
				stringstream(remote.substr(9)) >> hex >> port2;
		
				if(ip2 != 0){
				temp = new socket_info(ip1,port1,ip2,port2);
				temp->socket_is_udp = true;
				result->push_back(temp);
				}
			}
		} catch (...){
		}
	}
	file.close();


	return result;
}


//Please compile with -std=c++0x enabled
//Takes a pid and returns a list of connections
list<socket_info*>* getPID(unsigned int pid){
	DIR *dir;
	struct dirent *ent;

	set<unsigned int> inodes;
	//String for filepath manipuation
	//Look up the file descriptors open under /proc/$pid/fd
	string path = "/proc/";
	path += to_string(pid);
	path += "/fd/";
	dir = opendir(path.c_str());
	string temp = path;

	struct stat filestat;

	//dir traversal to check for all fd's
	if (dir != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			temp = path + ent->d_name;

			memset(&filestat,0,sizeof(filestat));
			stat(temp.c_str(),&filestat);

			//Record the inode if this is indeed a socket
			if(S_ISSOCK(filestat.st_mode)){
				inodes.insert(filestat.st_ino);
			}

		}
		closedir(dir);
	} else {
		cout << "Error: pid not found" << endl;
	}	

	return filterConnByInode(inodes);
}

int main(void){
	list<socket_info*>*l = getPID(17187);//Change this to some pid of your choice

	for(socket_info *k : *l){
		cout << hex << k->local_ip << endl;
	}
}
