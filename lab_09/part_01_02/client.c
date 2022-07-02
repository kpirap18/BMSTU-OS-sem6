#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#include "socket.h"

static int client_sock;
int sock;

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
	printf("\nClient: Catch SIGINT\n");
    cleanup(sock);
    exit(0);
}

int main(int argc, char* argv[]) 
{
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (sock == -1) 
    {
        perror("Client: socket call error");
        return errno;
    }
    client_sock = sock;

    if (signal(SIGINT, sighandler) == SIG_ERR) 
    {
        perror("Client: signal call error");
        cleanup(sock);
        return errno;
    }
    
    /* client socket */
    struct sockaddr_un addr_client;
    addr_client.sun_family = AF_UNIX;
    
    char full_path_client[BUF_SIZE] = {0};
    strcpy(full_path_client, SOCKET_PATH);
    strcat(full_path_client, SOCKET_NAME);
    // strcat(full_path_client, CLIENT_PREFIX);
    // printf("client fullname = %s\n", full_path_client);
    strcpy(addr_client.sun_path, full_path_client);
    
    if (bind(sock, (struct sockaddr*)&addr_client, sizeof(addr_client)) == -1) 
    {
        perror("Client: bind call error");
        cleanup(sock);
        return errno;
    }
    
    /* server socket */
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    char full_path[BUF_SIZE] = {0};
    strcpy(full_path, SOCKET_PATH);
    strcat(full_path, SOCKET_NAME);
    // printf("server fullname = %s\n", full_path);
    strcpy(addr.sun_path, full_path);


    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) 
    {
        perror("Client: Can't set dest address");
        return errno;
    }

    char msg[BUF_SIZE];
    if (argc > 1) 
    {
        snprintf(msg, BUF_SIZE, "[process %d]: %s", getpid(), argv[1]);
        strcat(msg, " Client");
    }
    else 
    {
        snprintf(msg, BUF_SIZE, "[process %d]: Hello", getpid());
        strcat(msg, " Client");
    }

    if (send(sock, msg, strlen(msg), 0) == -1) 
    { 
        perror("Client: sendto call error");
        return errno;
    } else 
    {
        printf("Client: sent message: '%s\'\n", msg);
    }

    char servermsg[BUF_SIZE] = {0};   

    printf("Client: waiting to receive...\n");

    size_t bytes = recv(sock, servermsg, sizeof(servermsg), 0);
    printf("Client: received!\n");
    
    
    if (bytes < 0) 
    {
        perror("Client: recv failed");
        return errno;
    } else 
    {
        printf("Client: received message: '%s'\n", servermsg);
    }
    printf("Client: closing...\n");
    cleanup(sock);
    return 0;
}
