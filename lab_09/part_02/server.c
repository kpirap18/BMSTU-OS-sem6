#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#define BUF_SIZE 256
#define PORT 3425
#define MAX_CLIENTS_NUMB 5


int sock_fd;


void del_socket(void)
{
    if (close(sock_fd) == -1) //закрытие сокета
    {
        printf("close() failed\n");
        return;
    }
}

void sigtstp_handler(int signum)
{
	printf("\nCatch SIGINT\n");
    del_socket();
    exit(0);
}



void receive_from_clients(int *clients, fd_set *fd_readset)
{
    char buf[BUF_SIZE];
    int bytes;

    for (int i = 0; i < MAX_CLIENTS_NUMB; i++)
    {
        // FD_ISSET проверяет, является ли описатель частью набора 
        if (FD_ISSET(clients[i], fd_readset))
        {
			// Поступили данные от клиента, читаем их
            bytes = recv(clients[i], buf, BUF_SIZE, 0);
	        if (bytes <= 0)
            {
				// Соединение разорвано, удаляем сокет из множества
				// (а если клиента завершать просто так, не закрывая его сокет, то мы никуда сюда не попадем)
		        close(clients[i]);
		        clients[i] = 0;
	        }
            else if (bytes > 0)
            {
                buf[bytes] = 0;
                printf("Server recieved from client %d: %s\n", i, buf);

                memset(buf, 0, BUF_SIZE);
                sprintf(buf, "Message recieved. You are number %d\n", i);
                send(clients[i], buf, sizeof(buf), 0);
            }
        }
    }
}

int main(void)
{
    struct sockaddr_in serv_addr;
    fd_set fd_readset;
    int clients[MAX_CLIENTS_NUMB] = {0};
	
	// AF_INET - семейство сетевых сокетов
	// SOCK_STREAM - для надежного упорядоченного полнодуплексного логического соединения
	// Потоковый сокет должен быть в состоянии соединения перед тем, как из него можно будет отсылать данные или принимать их.
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
		printf("socket() failed\n");
        return EXIT_FAILURE;
    }
	
	signal(SIGINT, sigtstp_handler); //изменение обработчика сигнала
	
	/*
	По умолчанию функция socket() создает блокирующий сокет (на время выполнения операции с таким сокетом ваша программа блокируется)
	
	Чтобы сделать его не-блокирующим (и использовать  select), надо использовать функцию fcntl(2) с флагом O_NONBLOCK.
	Теперь любой вызов функции read() для сокета sock_fd будет возвращать управление сразу же. 
	Если на входе сокета нет данных для чтения, функция read() вернет значение EAGAIN. 
	*/
    fcntl(sock_fd, F_SETFL, O_NONBLOCK);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; //сервер зарегистрируется на всех адресах той машины, на которой она выполняется.
    serv_addr.sin_port = htons(PORT);

	if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
      printf("bind() failed\n");
	  del_socket();
      return EXIT_FAILURE;
    }

    if (listen(sock_fd, MAX_CLIENTS_NUMB) == -1)
    {
        printf("listen() failed\n");
        del_socket();
		return EXIT_FAILURE;
    }

   
    printf("Listening.\nPress Ctrl + C ...\n");

    while(1)
    {
		// initialize the descriptor set to the null set. 
		FD_ZERO(&fd_readset);
		// FD_SET - добавление дескриптора в набор
		// слушающий сокет добавляется в набор дескрипторов, которые  следует проверять на готовность к чтению
		FD_SET(sock_fd, &fd_readset);
		
        int max_fd = sock_fd;
        for (int i = 0; i < MAX_CLIENTS_NUMB; i++)
        {
            if (clients[i])
            {
                FD_SET(clients[i], &fd_readset);
            }
		
            if (max_fd < clients[i])
			{
               max_fd = clients[i];
			}
         }
         
		/*
        select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
	 	Параметры:
		1 - количество проверяемых дескрипторов (на единицу больше самого большого номера описателей из всех наборов) 
		2, 3, 4 -  наборы дескрипторов, которые следует проверять, соответственно, на готовность к чтению, записи и на наличие исключительных ситуаций. 
		5 - можно указать интервал времени, по прошествии которого она вернет управление в любом случае. 
		Возвращаемое значение:
		значение больше нуля, которое соответствует числу готовых к операции дескрипторов. 
			А все дескрипторы, которые привели к "срабатыванию" функции, записываются в соответствующие множества.
		ноль -- в случае истечения тайм-аута, 
		отрицательное значение при ошибке. 

		Сама функция select() - блокирующая, она возвращает управление, если хотя бы один из проверяемых сокетов готов к выполнению соответствующей операции. 
        */
		// Ждём события в одном из сокетов
        if (select(max_fd + 1, &fd_readset, NULL, NULL, NULL) <= 0)
        {
            printf("select() failed\n");
            del_socket();
            return EXIT_FAILURE;
        }

		// Определяем тип события и выполняем соответствующие действия
		// FD_ISSET проверяет, является ли описатель частью набора
		if (FD_ISSET(sock_fd, &fd_readset))
		{
			// Поступил новый запрос на соединение (sock_fd - слушающий сокет)
			// блокируется, пока не появится "свисток" connect (NULL - адрес и длина адреса запросившего, неважны)
			int new_sock = accept(sock_fd, NULL, NULL);

			if (new_sock == -1)
			{
				printf("accept() failed\n");
				del_socket();
				return EXIT_FAILURE;
			}

			fcntl(new_sock, F_SETFL, O_NONBLOCK);

			int place_not_found = 1;
			for (int i = 0; i < MAX_CLIENTS_NUMB && place_not_found; i++)
			{
				if (!clients[i])
				{
					clients[i] = new_sock;
					printf("New connection %d\n", i);
					place_not_found = 0;
				}
			}
		}
		
		receive_from_clients(clients, &fd_readset);
	}
	return EXIT_SUCCESS;
}

