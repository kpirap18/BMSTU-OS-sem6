#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

void info()
{
	struct stat statbuf;

	stat("res3_th.txt", &statbuf);
	printf("inode: %ld\n", statbuf.st_ino);
	printf("Общий размер в байтах: %ld\n", statbuf.st_size);
	printf("Размер блока ввода-вывода: %ld\n\n", statbuf.st_blksize);
}

void run_buffer(char c)
{
	FILE *f = fopen("res3_th.txt", "a");
	info();

	while (c <= 'z')
	{
		fprintf(f, "%c", c);
		c += 2;
	}

	fclose(f);
	info();
}

void *for_help(void *arg)
{
	run_buffer('a');
}

int main()
{
    pthread_t thread;
    int rc = pthread_create(&thread, NULL, for_help, NULL);

    if (rc)
	{
		printf("не­воз­мож­но соз­дать по­ток");
		return -1;
	}

    sleep(1);
    run_buffer('b');

    pthread_join(thread, NULL);
    return 0;
}