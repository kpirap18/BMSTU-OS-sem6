#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SOCK_NAME "socket.soc"
#define BUF_SIZE 256



int main(void)
{	//домен соединения (семейство адресов) AF_UNIX: Сокеты в файловом пространстве имен
	//тип SOCK_DGRAM -  определяют ненадежную  службу datagram без установления логического соединения, когда пакеты могут передаваться без сохранения логического порядка
    // протокол - по умолчанию
	//создаёт конечную точку соединения и возвращает файловый дескриптор
	int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock_fd == -1)
    {
        printf("socket() failed");
        return EXIT_FAILURE;
    }
	
	//общая структура адреса (адрес в данном случае -- имя файла)
    struct sockaddr srvr_name;
    srvr_name.sa_family = AF_UNIX;
    strcpy(srvr_name.sa_data, SOCK_NAME);
	
	char buf[BUF_SIZE];
    sprintf(buf, "pid %d", getpid());
	
	// передача данных серверу. 4 параметр 0 - дополнительные флаги, далее адрес сервера и его длина
	if (sendto(sock_fd, buf, strlen(buf), 0, &srvr_name, strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family)) == -1)
    {
        printf("sendto() failed");
		close(sock_fd); //закрытие сокета
        return EXIT_FAILURE;
    }
	
	printf("Client sent: %s\n", buf);
	
    close(sock_fd);
    return EXIT_SUCCESS;
}

//адрес клиента не играет никакой роли, поэтому можно без bind (привязка сокета к локальному адресу)
