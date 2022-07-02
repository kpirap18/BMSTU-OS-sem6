#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "socket.h"

// Можно передать сообщение, как аргумент командной строки
// По умолчанию будет передаваться строка "Hello world".
int main(int argc, char *argv[])
{
	long int curr_time = time(NULL); // Считываем текущее время.
	struct sockaddr srvr_name; // Данные об адресе сервера.
	int sock;				   // Дескриптор сокета.

	char buf[MAX_MSG_LEN];

	// СОЗДАНИЕ СООБЩЕНИЯ
	sprintf(buf, "\n\nPID: %d\nTime: %s\nMessage: ", getpid(), ctime(&curr_time));

	if (argc < 2)
		strcat(buf, "Hello world!");
	else
		strcat(buf, argv[1]);
	// СОЗДАНИЕ СООБЩЕНИЯ

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket failed");
		return -1;
	}

	srvr_name.sa_family = AF_UNIX;
	strcpy(srvr_name.sa_data, SOCK_NAME);

	// 0 - доп флаги.
	if (sendto(sock, buf, strlen(buf) + 1, 0, &srvr_name, strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family)) < 0)
	{
		perror("sendto failed");
		return -1;
	}

	recvfrom(sock, buf, sizeof(buf), 0, &srvr_name, strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family));
	printf("Client: Data received: %s \n", buf);

	close(sock);
	printf("Send!");
	return OK;
}
