#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "socket.h" 

static int sock;

void cleanup(int sock)
{
	if (close(sock) == -1) //закрытие сокета
    {
        printf("close() failed");
        return;
    }
    char full_path[BUF_SIZE] = {0};
    strcpy(full_path, SOCKET_PATH);
    strcat(full_path, SOCKET_NAME);
    printf("cleanup fullname = %s\n", full_path);
    unlink(full_path);
}

void sighandler(int signum) 
{
	printf("\nCatch SIGINT\n");
    cleanup(sock);
    exit(0);
}

int main(void) 
{
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) 
    {
        perror("Server: socket call error");
        return errno;
    }

    struct sockaddr_un sock_addr;
    sock_addr.sun_family = AF_UNIX;
    char full_path[BUF_SIZE] = {0};
    strcpy(full_path, SOCKET_PATH);
    strcat(full_path, SOCKET_NAME);
    // printf("fullname = %s\n", full_path);

    strcpy(sock_addr.sun_path, full_path);

    if (bind(sock, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) == -1) 
    {
        perror("Server: bind call error");
         cleanup(sock);
        return errno;
    }

    if (signal(SIGINT, sighandler) == SIG_ERR) 
    {
        perror("Server: signal call error");
         cleanup(sock);
        return errno;
    }

    char msg[BUF_SIZE];
    char back[BUF_SIZE];
    while(1)
    {
        printf("Server: Waiting for client\n");
        struct sockaddr_un client_addr;
        int addr_size;
        size_t bytes = recvfrom(sock, msg, sizeof(msg) - 1, 0, (struct sockaddr*) &client_addr, (socklen_t*)&addr_size);
        
        if (bytes == -1) 
        {
            perror("Server: recv call error");
             cleanup(sock);
            return errno;
        }
        

        printf("Server: received message from %s: '%s'\n", client_addr.sun_path, msg);

        strcpy(back, msg);
        strcat(back, " Server");
        
        bytes = sendto(sock, back, strlen(back), 0, (struct sockaddr*) &client_addr, addr_size);

        printf("Server: sent message with size %ld: '%s' \n", bytes, back);
        
        if (bytes <= 0) 
        {
            printf("Server: sendto error");
            cleanup(sock);
            return errno;
        }
    }
}
