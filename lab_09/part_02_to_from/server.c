#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

#include "socket.h"

int sock;

#define MAX_COUNT_SOCK 2
// TODO: уменьшить MAX_COUNT_SOCK и проверить что будет...

// Структура для сетевого взаимодействия.
// struct sockaddr_in
// {
// 	short int sin_family;		 // Семейство адресов
// 	unsigned short int sin_port; // Номер порта (главное отличие от sockaddr)
// 	struct in_addr sin_addr;	 // IP-адрес хоста.
// 	unsigned char sin_zero[8];	 // Дополнение до размера структуры sockaddr
// };

// struct in_addr
// {
// 	unsigned long s_addr;
// };

void cleanup(int sock)
{
	if (close(sock) == -1) //закрытие сокета
    {
        printf("close() failed");
        return;
    }
}


void sighandler(int signum)
{
	printf("\nCatch SIGINT\n");
    cleanup(sock);
    exit(0);
}

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	int clen;
	struct sockaddr_in cli_addr;

	fd_set set; // Набор дескрипторов.
	int max_sock;

	int clients[MAX_COUNT_SOCK] = {0};

	// Структура специально предназначенная
	// для хранения адресов в формате Интернета.
	struct sockaddr_in serv_addr =
		{
			.sin_family = AF_INET,
			// INADDR_ANY - зарегистрировать нашу программу-сервер
			// на всех адресах машины, на которой она выполняется.
			.sin_addr.s_addr = INADDR_ANY,
			// htons() переписывает двухбайтовое значение порта так,
			// чтобы порядок байтов соответствовал сетевому.
			.sin_port = htons(PORT)};

	

	printf("htons(PORT) = %d\n", htons(PORT));

	// AF_INET - открываемый сокет должен быть сетевым.
	// SOCK_STREAM - требование, чтобы сокет был потоковым.
	// 0 - протокол выбирается по умолчанию.
	sock = socket(AF_INET, SOCK_STREAM, 0);
	max_sock = sock;

	if (sock < 0)
	{
		printf("socket() failed: %d\n", errno);
		return -1;
	}

	fcntl(sock, F_SETFL, O_NONBLOCK);

	// bind() - связывает сокет с заданным адресом.
	// После вызова bind() программа-сервер становится доступна для соединения по заданному адресу (имени файла)
	if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		printf("bind() failed: %d\n", errno);
		return -1;
	}

	signal(SIGINT, sighandler); 

	// listen переводит сервер в режим ожидания запроса на соединение.
	// Второй параметр - максимальное число соединений, которые сервер может обрабатывать одновременно.
	if (listen(sock, 3) == -1)
	{
		printf("listen() failed: %d\n", errno);
		return -1;
	}

	printf("Сервер работает!\n");
	struct timeval timeout = {9, 0}; // 9 sec
	while (TRUE)
	{
		// Очищает набор.
		FD_ZERO(&set);
		// Добавляет sock к набору.
		FD_SET(sock, &set);

		max_sock = sock; // max_sock нужен для select'а.

		for (int i = 0; i < MAX_COUNT_SOCK; i++)
		{
			if (clients[i])
			{
				// Добавляем в набор
				FD_SET(clients[i], &set);
			}
			max_sock = clients[i] > max_sock ? clients[i] : max_sock;
		}

		// первый аргумент на единицу больше самого большого номера описателей из всех наборов.
		// При возврате из функции select наборы описателей модифицируются, чтобы показать, какие описатели фактически изменили свой статус.
		// Вызов select() (или pselect()) используется для эффективного слежения за несколькими 
		// файловыми дескрипторами — для ожидания, когда какой-то из них не станет «готов», 
		// то есть появится возможность чтения-записи данных, или с файловым дескриптором не возникнет «исключительная ситуация».
		int retval = select(max_sock + 1, &set, NULL, NULL, &timeout);
		if (retval == -1)
		{
			printf("select() failed: %d\n", errno);
			return -1;
		}

		// Проверка на новое подключение.
		// возвращает ненулевое значение, если указанный файловый дескриптор 
		// присутствует в наборе и ноль, если отсутствует.
		if (FD_ISSET(sock, &set))
		{
			// Если sock остался в set, значит он изменил свой статус
			// (к нему кто-то подключился) и значит он ожидает обработки.
			printf("New connetcion\n");

			// accept() устанавливает соединение в ответ на запрос клиента и создает
			// копию сокета для того, чтобы исходный сокет мог продолжать прослушивание.
			// Сервер перенаправляет запрошенное соединение на другой сокет (newsock), оставляя
			// сокет sock свободным для прослушивания запросов на установку соединения.
			int newsock = accept(sock, NULL, NULL); // (struct sockaddr *)&cli_addr, &clen);
			
			if (newsock == -1)
			{
				printf("accept() failed: %d\n", errno);
				return -1;
			}

			fcntl(newsock, F_SETFL, O_NONBLOCK);

			int flag = 1;
			for (int i = 0; i < MAX_COUNT_SOCK && flag; i++)
			{
				if (clients[i] == 0)
				{
					clients[i] = newsock;
					printf("Клиент номер %d\n", i);
					flag = 0;
				}
			}
			if (flag)
			{
				printf("Больше нет места для новых клиентов.\n");
			}
		}

		// Проверяем, послали ли клиенты сообщения.
		for (int i = 0; i < MAX_COUNT_SOCK; i++)
		{
			if (clients[i] && FD_ISSET(clients[i], &set))
			{
				// recvfrom - получить сообщение из сокета.
				// Возвращает кол-во принятых байт.
				// -1, если ошибка.

				char buf[MAX_LEN_BUF];

				int rv = recv(clients[i], buf, sizeof(buf), 0);
				
				if (rv == 0)
				{
					printf("Отключение от сервера клиента номер %d\n", i);
					close(clients[i]);
					clients[i] = 0;
				}
				else
				{
					char *str_res;
					int number = atoi(buf);
					if (number)
					{
						sprintf(str_res, "Клиент номер %d Result: %d\n", i, number * number);
					}
					else
					{
						sprintf(str_res, "Клиент номер %d. \"%s", i, buf);
					}

					printf("Client[%d] sent %s\n", i, buf);
                	send(clients[i], buf, rv, 0);
				}
			}
		}
	}

	close(sock);
	return 0;
}

// искать примеры с мультиплексерованием