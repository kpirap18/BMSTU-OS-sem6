#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

void info()
{
	struct stat statbuf;

	stat("res3.txt", &statbuf);
	printf("inode: %ld\n", statbuf.st_ino);
	printf("Общий размер в байтах: %ld\n", statbuf.st_size);
	printf("Размер блока ввода-вывода: %ld\n\n", statbuf.st_blksize);
}

int main()
{
    FILE *f1 = fopen("res3.txt", "a");
    info();
    FILE *f2 = fopen("res3.txt", "a");
    info();


	printf("\nfs1 _fileno: %d", f1->_fileno);
	printf("\nfs2 _fileno: %d\n\n", f2->_fileno);

    for (char c = 'a'; c <= 'z'; c++)
    {
        if (c % 2)
        {
            fprintf(f1, "%c", c);
        }
        else
        {
            fprintf(f2, "%c", c);
        }
    }

    fclose(f2);
    info();
    fclose(f1);
    info();

    return 0;
}