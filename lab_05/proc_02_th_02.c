#include <fcntl.h>
#include <unistd.h> // read, write.
#include <pthread.h>
#include <stdio.h>

#define GREEN "\33[32m"
#define BLUE "\33[34m"

static int pos = 0;
pthread_mutex_t mutex;


int read_file(int fd, int flag)
{
    int rc;
	// pthread_mutex_lock(&mutex);
    char c;
    int local = pos;
    lseek (fd, local, SEEK_SET);
    if ((rc = read(fd, &c, 1)) == 1) //while (local < 26) 
    {
        // printf("  pos %d  %d\n", local, local < 26);
        lseek (fd, local, SEEK_SET);
        if (flag == 2)
        {
            c = c + 32;
        }
        write(1, &c, 1);
        local++;
    }
    // printf("%d ", pos);
    pos = local;
	// pthread_mutex_unlock(&mutex);

    return rc;
}

void *thr_fn(void *arg)
{
	int fd = open("alphabet.txt", O_RDONLY);
    int rc = 1;
    while (rc == 1)
    {
	    rc = read_file(fd, 2);
    }
}

int main()
{
	pthread_t tid;
    int rc = 1;

	int fd = open("alphabet.txt", O_RDONLY);

	// NULL - атрибуты по умолчанию
	// arg = 0 не передаем аргументы.
	int err = pthread_create(&tid, NULL, thr_fn, 0);
	if (err)
	{
		printf("не­воз­мож­но соз­дать по­ток");
		return -1;
	}

	
	pthread_join(tid, NULL);
    
    while (rc == 1)
    {
	    rc = read_file(fd, 2);
    }

    write(1, "\n", 1);
	return 0;
}