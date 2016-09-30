#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../common/tcp.h"

// function prototypes
void passive_mode(uint16_t port);
void active_mode(const ip_port& dest);
void* listen_thread(void* args);
void keyboard_loop(tcp& connection);

// tcp port chatter
int main(int argc, char** argv)
{
	bool passive = true;
	uint16_t port = 0;
	ip_port destination;

	// parse arguments
	if (argc != 2)
		goto error;

	if (ip_port::validate_ip_port(argv[1]))
	{
		destination.set(argv[1]);
		printf("dest addr %s port %d\n", destination.get_address().to_string().c_str(), destination.get_port());
		passive = false;
	}
	else if ((sscanf(argv[1], "%hu", &port) != 1) || (port == 0))
		goto error;

	// branch on mode
	passive ? passive_mode(port) : active_mode(destination);
	return 0;

error:

	printf("usage in active mode : %s [dest IP address]:[port]\neg. %s 10.0.0.2:8888\n", argv[0], argv[0]);
	printf("usage in passive mode: %s [listen port]\neg. %s 8080\n", argv[0], argv[0]);
	return 1;
}

// passive mode connector
void passive_mode(uint16_t port)
{
	pthread_t tid;
	tcp connection;

	printf("listening on port %hu.\n", port);
	if (!tcp::listen(port))
	{
		printf("error: unable to listen on port %hu.\n", port);
		return;
	}

	if (!connection.accept(port))
	{
		printf("error: unable to accept connection.\n");
		return;
	}
	printf("local  IP/port: %s\n", connection.get_connection_info().get_local_ip_port().to_string().c_str());
	printf("remote IP/port: %s\n", connection.get_connection_info().get_remote_ip_port().to_string().c_str());

	printf("connected. type and hit enter to send.\n");

	// fork off listen thread
	pthread_create(&tid, NULL, listen_thread, &connection);
	keyboard_loop(connection);
}

// active mode connector
void active_mode(const ip_port& destination)
{
	pthread_t tid;
	tcp connection;
	printf("connecting...\n");
	if (!connection.connect(destination))
	{
		printf("unable to connect.\n");
		return;
	}

	printf("local  IP/port: %s\n", connection.get_connection_info().get_local_ip_port().to_string().c_str());
	printf("remote IP/port: %s\n", connection.get_connection_info().get_remote_ip_port().to_string().c_str());

	printf("connected. type and hit enter to send.\n");

	// fork off listen thread
	pthread_create(&tid, NULL, listen_thread, &connection);
	keyboard_loop(connection);
}

// listen thread
void* listen_thread(void* args)
{
	char buf[1024];
	int buf_used = 0;
	tcp* connection = (tcp*) args;

	while(1)
	{
		if (connection->recv_raw(buf, &buf_used, 128))
		{
			printf("[remote message] %s", buf);
			if (buf_used > 0 && buf[buf_used-1] != '\n')
				printf("\n");

			for (uint32_t counter = 0; counter < (uint32_t) buf_used; ++counter) {
				printf("[%d] ", buf[counter]);
			}
			printf("\n");

		}
		else
		{
			printf("\n\n[error: connection closed]\n");
			exit(0);
		}
	}

	return NULL;
}

// keyboard input loop
void keyboard_loop(tcp& connection)
{
	char buf[1024];
	while(1)
	{
		fgets(buf, sizeof(buf), stdin);
		if (strlen(buf) == 0)
		{
			printf("\n[empty buffer; content not sent]\n");
			continue;
		}
		if (!connection.send_raw(buf, strlen(buf)))
		{
			printf("\n\n[error: connection closed]\n");
			return;
		}
	}
}
