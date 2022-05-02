#include <fcntl.h>
#include <unistd.h> // read, write.
#include <pthread.h>
#include <stdio.h>

static int pos = 0;
pthread_mutex_t mutex;


void read_file(int fd, int flag)
{
	char c;
	// lseek (fd, pos, SEEK_SET);
	while (pos < 26)
	{
		pthread_mutex_lock(&mutex);
			lseek (fd, pos, SEEK_SET);
			read(fd, &c, 1);
			// printf(" %d -- ", pos);
			
			if (flag == 2)
			{
				c = c + 32;
			}
			write(1, &c, 1);
			pos++;
		pthread_mutex_unlock(&mutex);
	}
}

void *thr_fn(void *arg)
{
	int fd = open("alphabet.txt", O_RDONLY);
	read_file(fd, 2);
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

	read_file(fd, 1);
	pthread_join(tid, NULL);

    write(1, "\n", 1);
	return 0;
}