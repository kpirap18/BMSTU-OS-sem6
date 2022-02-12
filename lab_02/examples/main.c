#include "apue.h"
#include <dirent.h>
#include <limits.h>

typedef int Myfunc(const char *, const struct stat *, int);

static Myfunc myfunc;

static int myftw(char *, Myfunc *);
static int dopath(Myfunc *);

static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;

int main(int argc, char *argv[])
{
	int ret;
	if (argc != 2)
	{
		printf("Укажите в аргументах командной строки начальный каталог.");
		return 1;
	}

	ret = myftw(argv[1], myfunc); /* вы­пол­ня­ет всю ра­бо­ту */

	ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
	if (ntot == 0)
		ntot = 1; /* во из­бе­жа­ние де­ле­ния на 0; вы­вес­ти 0 для всех счет­чи­ков */
	printf("Обычные файлы = %7ld, %5.2f %%\n", nreg, nreg * 100.0 / ntot);
	printf("Каталоги = %7ld, %5.2f %%\n", ndir, ndir * 100.0 / ntot);
	printf("Специальные файлы блочных устройств = %7ld, %5.2f %%\n", nblk, nblk * 100.0 / ntot);
	printf("Специальные файлы символьных устройств = %7ld, %5.2f %%\n", nchr, nchr * 100.0 / ntot);
	printf("FIFO = %7ld, %5.2f %%\n", nfifo, nfifo * 100.0 / ntot);
	printf("Символические ссылки = %7ld, %5.2f %%\n", nslink, nslink * 100.0 / ntot);
	printf("Сокеты = %7ld, %5.2f %%\n", nsock, nsock * 100.0 / ntot);

	exit(ret);
}

#define FTW_F 1	  // файл, не являющийся каталогом
#define FTW_D 2	  // каталог
#define FTW_DNR 3 // каталог, недоступный для чтения
#define FTW_NS 4  // файл, информацию о котором нельзя получить с помощью stat

static char *fullpath; /* пол­ный путь к каж­до­му из фай­лов */
static size_t pathlen;

/* воз­вра­ща­ем то, что вер­ну­ла функ­ция func() */
static int myftw(char *pathname, Myfunc *func)
{
	// fullpath = path_alloc(&len); /* вы­де­лить па­мять для PATH_MAX+1 байт */
	if (pathlen <= strlen(pathname))
	{
		printf("%s\n", pathname);
		printf("len = %ld", strlen(pathname));
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("ошиб­ка вы­зо­ва realloc");
	}
	strcpy(fullpath, pathname);
	printf("%s\n", fullpath);

	return (dopath(func));
}

/* воз­вра­ща­ем то, что вер­ну­ла функ­ция func() */
static int dopath(Myfunc *func)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	int ret, n;

	if (lstat(fullpath, &statbuf) < 0) /* ошиб­ка вы­зо­ва функ­ции stat */
		return (func(fullpath, &statbuf, FTW_NS));
	if (S_ISDIR(statbuf.st_mode) == 0) /* не ка­та­лог */
		return (func(fullpath, &statbuf, FTW_F));
	/*
* Это ка­та­лог. Сна­ча­ла вы­зо­вем функ­цию func(),
* а за­тем об­ра­бо­та­ем все фай­лы в этом ка­та­ло­ге.
*/
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return (ret);
	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen)
	{ /* уве­ли­чить раз­мер бу­фе­ра */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("ошиб­ка вы­зо­ва realloc");
	}
	fullpath[n++] = '/';
	fullpath[n] = 0;
	if ((dp = opendir(fullpath)) == NULL) /* ка­та­лог не­дос­ту­пен */
		return (func(fullpath, &statbuf, FTW_DNR));
	while ((dirp = readdir(dp)) != NULL)
	{
		if (strcmp(dirp->d_name, ".") == 0 ||
			strcmp(dirp->d_name, "..") == 0)
			continue;						/* про­пус­тить ка­та­ло­ги "." и ".." */
		strcpy(&fullpath[n], dirp->d_name); /* до­ба­вить имя по­сле слэ­ша */
		
		if ((ret = dopath(func)) != 0)
			/* ре­кур­сия */
			break; /* вы­ход по ошиб­ке */
	}
	fullpath[n - 1] = 0; /* сте­реть часть стро­ки от слэ­ша и до кон­ца */
	if (closedir(dp) < 0)
		exit(1);
	// err_ret("не­воз­мож­но за­крыть ка­та­лог %s", fullpath);
	return (ret);
}

static int myfunc(const char *pathname, const struct stat *statptr, int type)
{
	switch (type)
	{
	case FTW_F:
		switch (statptr->st_mode & S_IFMT)
		{
		case S_IFREG:
			nreg++;
			break;
		case S_IFBLK:
			nblk++;
			break;
		case S_IFCHR:
			nchr++;
			break;
		case S_IFIFO:
			nfifo++;
			break;
		case S_IFLNK:
			nslink++;
			break;
		case S_IFSOCK:
			nsock++;
			break;
		case S_IFDIR: /* ка­та­ло­ги долж­ны иметь type = FTW_D*/
			exit(2);
			// err_dump("при­знак S_IFDIR для %s", pathname);
		}
		break;
	case FTW_D:
		ndir++;
		break;
	case FTW_DNR:
		exit(11);
		// err_ret("за­крыт дос­туп к ка­та­ло­гу %s", pathname);
		break;
	case FTW_NS:
		exit(12);
		// err_ret("ошиб­ка вы­зо­ва функ­ции stat для %s", pathname);
		break;
	default:
		exit(13);
		// err_dump("не­из­вест­ный тип %d для фай­ла %s", type, pathname);
	}
	return (0);
}
