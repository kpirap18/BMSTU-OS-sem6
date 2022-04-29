#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/fs.h>

#define OK 0
#define FILE_NAME "alphabet.txt"
#define BUFF_SIZE 15

#define GREEN "\33[32m"
#define BLUE "\33[34m"

int main()
{
	int fd = open(FILE_NAME, O_RDONLY); // O_RDONLY - только на чтение.

	FILE *fs1 = fdopen(fd, "r");
	char buff1[BUFF_SIZE];
	// Функция изменяет буфер, который будет использоваться для операций ввода/вывода с указанным потоком. 
	// Эта функция позволяет задать режим доступа и размер буфера. 
	setvbuf(fs1, buff1, _IOFBF, BUFF_SIZE); // _IOFBF - блочная буферизация.

	FILE *fs2 = fdopen(fd, "r");
	char buff2[BUFF_SIZE];
	setvbuf(fs2, buff2, _IOFBF, BUFF_SIZE);

	int flag1 = 1, flag2 = 2;

	printf("\nfs1 _fileno: %d\n", fs1->_fileno);
	printf("\nfs2 _fileno: %d\n", fs2->_fileno);
	printf("\nfs1 buff1[0] == fs1->_IO_buf_base: %d\n", buff1 == fs1->_IO_buf_base);
	printf("\nfs2 buff2[0] == fs2->_IO_buf_base: %d\n", buff2 == fs2->_IO_buf_base);

	while (flag1 == 1 || flag2 == 1)
	{
		char c;
		flag1 = fscanf(fs1, "%c", &c);
		if (flag1 == 1)
		{
			fprintf(stdout, GREEN "%c", c);
		}
		flag2 = fscanf(fs2, "%c", &c);
		if (flag2 == 1)
		{
			fprintf(stdout, BLUE "%c", c);
		}
	}

	printf("\n");
	return OK;
}