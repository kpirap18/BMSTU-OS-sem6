#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

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
	struct sockaddr srvr_name; // Данные об адресе сервера.
	struct sockaddr rcvr_name; // Данные об адресе клиента, запросившего соединение
	int namelen;			   // Длина возвращаемой структуры с адресом.

	char buf[MAX_MSG_LEN]; // Буфер, в который записываются сообщения от клиентов.
	
	int bytes;			   // Кол-во полученных байт.

	long int curr_time = time(NULL); // Считываем текущее время.

	// Создаем сокет. Получаем дескриптор сокета.
	// AF_UNIX - сокеты в файловом пространстве имен.
	// SOCK_DGRAM - датаграмный сокет. Хранит границы сообщений.
	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		perror("Server: socket failed");
		return -1;
	}

	srvr_name.sa_family = AF_UNIX;
	strcpy(srvr_name.sa_data, SOCK_NAME); // sa_data - имя файла сокета.

	signal(SIGINT, sighandler);
	printf("Press Ctrl + C to stop...\n");


	// bind() - связывает сокет с заданным адресом.
	// После вызова bind() программа-сервер становится доступна для соединения по заданному адресу (имени файла)
	if (bind(sock, &srvr_name, strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family)) == -1)
	{
		perror("bind failed");
		return -1;
	}

	while (TRUE)
	{
		// recvfrom блокирует программу до тех пор, пока на входе не появятся новые данные.
		// bytes = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL); // В нашем случае можно и вот так.
		bytes = recvfrom(sock, buf, sizeof(buf), 0, &rcvr_name, &namelen);

		if (bytes < 0)
		{
			perror("recvfrom failed");
			cleanup(sock);
			return -1;
		}

		if(sendto(sock, buf, strlen(buf)*sizeof(char), 0, &rcvr_name, &namelen) == -1 )
		{
			printf("Server: Error on send() call \n");
		}

		printf("\n\nReceived message: %s\nLen = %ld\n", buf, strlen(buf));
	}

	cleanup(sock);

	return OK;
}
