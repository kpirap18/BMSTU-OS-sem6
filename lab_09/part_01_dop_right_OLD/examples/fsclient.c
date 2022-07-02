#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#define SOCK_NAME "socket.soc"

int main()
{
	struct sockaddr srvr_name;
	char buf[32];

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket failed");
		return -1;
	}

	srvr_name.sa_family = AF_UNIX;
	strcpy(srvr_name.sa_data, SOCK_NAME);
	strcpy(buf, "Hello, Unix sockets!");
	// 0 - доп флаги.
	// srvr_name - информация об адресе сервера.
	sendto(sock, buf, strlen(buf) + 1, 0, &srvr_name,
		   strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family));

	printf("Client say: OK!");

	close(sock);

	return 0;
}