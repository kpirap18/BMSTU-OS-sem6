#include <fcntl.h>
#include <unistd.h> // read, write.
#include <pthread.h>
#include <stdio.h>

void read_file(int fd)
{
	char c;
	while (read(fd, &c, 1))
	{
		write(1, &c, 1);
	}
}

void *thr_fn(void *arg)
{
	int fd = open("alphabet.txt", O_RDONLY);
	read_file(fd);
}

int main()
{
	pthread_t tid;

	int fd = open("alphabet.txt", O_RDONLY);

	// NULL - атрибуты по умолчанию
	// arg = 0 не передаем аргументы.
	int err = pthread_create(&tid, NULL, thr_fn, 0);
	if (err)
	{
		printf("не­воз­мож­но соз­дать по­ток");
		return -1;
	}

	read_file(fd);
	pthread_join(tid, NULL);

    write(1, "\n", 1);
	return 0;
}