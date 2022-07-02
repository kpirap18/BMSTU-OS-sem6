#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "socket.h"


// static int client_sock;
static int sock;

void cleanup(int sock)
{
	if (close(sock) == -1) //закрытие сокета
    {
        printf("close() failed");
        return;
    }

    char full_path[BUF_SIZE] = {0};
    strcat(full_path, CLIENT_NAME);
    // printf("cleanup fullname = %s\n", full_path);

    if (unlink(full_path) == -1) //удаление файла сокета
    {
        printf("unlink() returned -1");
    }
}

void sighandler(int signum) 
{
	printf("\nClient: Catch SIGINT\n");
    cleanup(sock);
    exit(0);
}

int main(int argc, char* argv[]) 
{
    size_t bytes;  
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock == -1) 
    {
        perror("Client: socket call error");
        return errno;
    }
    printf("Client: socket called\n");


    if (signal(SIGINT, sighandler) == SIG_ERR) 
    {
        perror("Client: signal call error");
        cleanup(sock);
        return errno;
    }

    char msg[BUF_SIZE];
    if (argc > 1) 
    {
        snprintf(msg, BUF_SIZE, "[process %d]: %s", getpid(), argv[1]);
        // strcat(msg, " from Client");
    }
    else 
    {
        snprintf(msg, BUF_SIZE, "[process %d]: Hello", getpid());
        // strcat(msg, " from Client");
    }
    
    /* client socket */
    char str_pid[10];
    snprintf(str_pid, BUF_SIZE, "pid_%d", getpid());

    // printf("!!!%d\n", sprintf(str_pid, "%d", getpid()));
    struct sockaddr_un addr_client;
    addr_client.sun_family = AF_UNIX;
    
    strcpy(addr_client.sun_path, CLIENT_NAME);
    strcat(addr_client.sun_path, str_pid);
    
    if (bind(sock, (struct sockaddr*)&addr_client, sizeof(addr_client)) == -1) 
    {
        perror("Client: bind call error");
        cleanup(sock);
        return errno;
    }
    printf("Client: bind called\n");
    
    /* server socket */
    struct sockaddr_un server_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_NAME);


    // if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) 
    // {
    //     perror("Client: Can't set dest address");
    //     return errno;
    // }

    // while(1)
    // {
        
        if (sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
        { 
            perror("Client: sendto call error");
            return errno;
        } 
        else 
        {
            printf("Client: sent message: '%s\'\n", msg);
        }


    // if (send(sock, msg, strlen(msg), 0) == -1) 
    // { 
    //     perror("Client: sendto call error");
    //     return errno;
    // } 
    // else 
    // {
    //     printf("Client: sent message: '%s\'\n", msg);
    // }

        char servermsg[BUF_SIZE] = {0};   

        printf("Client: waiting to receive...\n");
        bytes = recv(sock, servermsg, sizeof(servermsg), 0);
        printf("%d \n", bytes);
        if(bytes > 0)
		{
			printf("Client: Data received: %s \n", servermsg);
		}
		else
		{
			if(bytes < 0)
			{
				printf("Client: Error on recv() call \n");
                return errno;
			}
			else
			{
				printf("Client: Error on recv() call \n");
			}
		}
		// sleep(3);
    // }

    printf("Client: closing...\n");
    cleanup(sock);
    return 0;
}
