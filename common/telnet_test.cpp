#include "switch_telnet.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {

	switch_telnet client;

	printf("connecting to 10.22.250.11...\n");
	if (!client.connect(ip_port("10.22.250.11", 23), "admin", "Dellsvcs1")) {
		printf("unable to connect.\n");
		return 1;
	} else {
		printf("connected and authenticated!\n");
	}
	sleep(1);
	client.write("\n", 1);

	char buf[1048576];
	char cmd[1024];
	sprintf(cmd, "terminal length 0\n");
	client.write(cmd, strlen(cmd));

//	sprintf(cmd, "show openflow flows of-instance 1\n");
	sprintf(cmd, "show vlan\n");
	client.write(cmd, strlen(cmd));
	int buf_used = 0;
	client.read_until_timeout(buf, sizeof(buf)-1, 500, &buf_used);
	buf[buf_used] = 0;

	printf("%s\n", buf);
	return 0;
}
