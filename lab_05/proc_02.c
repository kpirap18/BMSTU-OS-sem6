#include <fcntl.h>
#include <unistd.h> // read, write.

// write записывает до count байтов из буфера buf в файл, 
// на который ссылается файловый описатель fd.
int main()
{
	char c;

	int fd1 = open("alphabet.txt", O_APPEND);
	int fd2 = open("alphabet.txt", O_APPEND);
    int rc1, rc2 = 1;

    while (rc1 == 1 || rc2 == 1)
    {
        char c;

        rc1 = read(fd1, &c, 1);
        if (rc1 == 1)
        {
            write(1, &c, 1);
        }

        rc2 = read(fd2, &c, 1);
        if (rc2 == 1)
        {
            write(1, &c, 1);
        }
    }

	write(1, "\n", 1);
	return 0;
}