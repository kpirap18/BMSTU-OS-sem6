#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "socket.h"

int sock;
void sighandler(int signum)
{
	printf("\nCatch SIGINT\n");
    close(sock);
    exit(0);
}

// Можно передать сообщение, как аргумент командной строки
// По умолчанию будет передаваться строка "Hello world".
int main(int argc, char *argv[])
{
	long int curr_time = time(NULL); // Считываем текущее время.

	struct sockaddr_un srvr_name;       // Данные об адресе сервера.
	int bytes = 0;

	char recv_msg[100];
	char send_msg[100];

	memset(recv_msg, 0, 100*sizeof(char));
	memset(send_msg, 0, 100*sizeof(char));

	// СОЗДАНИЕ СООБЩЕНИЯ
	sprintf(send_msg, "\nPID: %d Message: ", getpid());

	if (argc < 2)
		strcat(send_msg, "Hello world!");
	else
		strcat(send_msg, argv[1]);
	// СОЗДАНИЕ СООБЩЕНИЯ

	while(TRUE)
	{
		sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (sock == -1)
		{
			perror("Client: socket failed");
			return -1;
		}

		srvr_name.sun_family = AF_UNIX;
		strcpy(srvr_name.sun_path, SOCK_NAME);

		printf("\nClient: Trying to connect... \n");
		if( connect(sock, (struct sockaddr*)&srvr_name, strlen(srvr_name.sun_path) + sizeof(srvr_name.sun_family)) == -1 )
		{
			printf("Client: Error on connect call \n");
			close(sock);
			return -1;
		}
		printf("Client: Connected \n");


		if (send(sock, send_msg, strlen(send_msg) + 1, 0) == -1)
		{
			perror("Client: sendto failed");
		}

		memset(recv_msg, 0, 100*sizeof(char));

		if((bytes = recv(sock, recv_msg, 100, 0)) > 0)
		{
			printf("Client: Data received: %s \n", recv_msg);
		}
		else
		{
			if(bytes < 0)
			{
				printf("Client: Error on recv() call \n");
			}
			else
			{
				printf("Client: Error on recv() call \n");
				close(sock);
				break;
			}
		}
		sleep(3);
	}

	printf("Client all!");
	return OK;
}
