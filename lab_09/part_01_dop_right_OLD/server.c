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
int sock;			   // Дескриптор сокета.

void cleanup(int sock)
{
	if (close(sock) == -1) //закрытие сокета
    {
        printf("close() failed");
        return;
    }
    if (unlink(SOCK_NAME) == -1) //удаление файла сокета
    {
        printf("unlink() returned -1");
    }
}


void sighandler(int signum)
{
	printf("Catch SIGINT\n");
    cleanup(sock);
    exit(0);
}

int main(int argc, char *argv[])
{
	int sock2;
	signal(SIGINT, sighandler);

	struct sockaddr_un srvr_name; // Данные об адресе сервера.
	struct sockaddr_un rcvr_name; // Данные об адресе клиента, запросившего соединение
	int namelen;			   // Длина возвращаемой структуры с адресом.
	
	int bytes;			   // Кол-во полученных байт.

	long int curr_time = time(NULL); // Считываем текущее время.

	// Создаем сокет. Получаем дескриптор сокета.
	// AF_UNIX - сокеты в файловом пространстве имен.
	// SOCK_DGRAM - датаграмный сокет. Хранит границы сообщений.
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket failed");
		return -1;
	}

	srvr_name.sun_family = AF_UNIX;
	strcpy(srvr_name.sun_path, SOCK_NAME); 


	printf("Press Ctrl + C to stop...\n");

	// bind() - связывает сокет с заданным адресом.
	// После вызова bind() программа-сервер становится доступна для соединения по заданному адресу (имени файла)
	if (bind(sock, (struct sockaddr*)&srvr_name, strlen(srvr_name.sun_path) + sizeof(srvr_name.sun_family)) == -1)
	{
		perror("Server: bind failed");
		cleanup(sock);
		return -1;
	}

	if( listen(sock, 5) == -1)
	{
		printf("Server: Error on listen call \n");
		cleanup(sock);
		return -1;
	}

	printf("Server: Listening... \n");

	while (TRUE)
	{
		unsigned int sock_len = 0;
		if( (sock2 = accept(sock, (struct sockaddr*)&rcvr_name, &sock_len)) == -1 )
		{
			printf("Error on accept() call \n");
			return -1;
		}

		printf("Server connected \n");

		char recv_buf[100];
		char send_buf[100];

		memset(recv_buf, 0, 100*sizeof(char));
		memset(send_buf, 0, 100*sizeof(char));

		bytes = recv(sock2, recv_buf, 100, 0);

		if(bytes > 0)
		{
			printf("\nServer: received: %d : %s \n", bytes, recv_buf);

			strcpy(send_buf, "Server got message!");
			strcat(send_buf, recv_buf);

			if(send(sock2, send_buf, strlen(send_buf)*sizeof(char), 0) == -1 )
			{
				printf("Server: Error on send() call \n");
			}
		}
		else
		{
			printf("Server: Error on recv() call \n");
			break;
		}


		close(sock2);
	}

	cleanup(sock);

	return OK;
}
