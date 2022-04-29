#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>

#define OK 0
#define BUF_SIZE 20
#define VALID_READED 1


void *run_buffer(void *args)
{
    int flag = 1;
    FILE *fs = (FILE *)args;

    while (flag == VALID_READED)
    {
        char c;
        if ((flag = fscanf(fs, "%c\n", &c)) == VALID_READED)
        {
            fprintf(stdout, "thread 2: " "%c\n", c);
        }
    }

    return NULL;
}

int main(void)
{
    setbuf(stdout, NULL);

    pthread_t thread;
    int fd = open("alphabet.txt", O_RDONLY); // O_RDONLY - только на чтение.

    FILE *fs1 = fdopen(fd, "r");
    char buff1[BUF_SIZE];
    setvbuf(fs1, buff1, _IOFBF, BUF_SIZE); // _IOFBF - блочная буферизация.

    FILE *fs2 = fdopen(fd, "r");
    char buff2[BUF_SIZE];
    setvbuf(fs2, buff2, _IOFBF, BUF_SIZE);

    int rc = pthread_create(&thread, NULL, run_buffer, (void *)fs2);

    int flag = 1;
    while (flag == VALID_READED)
    {
        char c;
        flag = fscanf(fs1, "%c\n", &c);
        if (flag == VALID_READED)
        {
            fprintf(stdout, "thread 1: " "%c\n", c);
        }
    }

    pthread_join(thread, NULL);

    return OK;
}