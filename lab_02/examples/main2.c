#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

#define FTW_F   1       
#define FTW_D   2       
#define FTW_DNR 3       
#define FTW_NS  4       

typedef struct
{
    long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;
} statistics;
static statistics stats;

void incStats(int type, statistics *stats)
{
    switch (type & S_IFMT)
    {
    case S_IFREG:
        stats->nreg++;
        break;
    case S_IFBLK:
        stats->nblk++;
        break;
    case S_IFCHR:
        stats->nchr++;
        break;
    case S_IFIFO:
        stats->nfifo++;
        break;
    case S_IFLNK:
        stats->nslink++;
        break;
    case S_IFSOCK:
        stats->nsock++;
        break;
    }
}

void printStats(statistics *stats)
{
    stats->ntot = stats->nreg + stats->ndir + stats->nblk + stats->nchr + stats->nfifo + stats->nslink + stats->nsock;
    if (!stats->ntot)
        stats->ntot = 1;
        
    printf(YELLOW);
    printf("Обычные файлы:                          %-7ld %-5.2f %%\n", stats->nreg, stats->nreg * 100.0 / stats->ntot);
    printf("Каталоги:                               %-7ld %-5.2f %%\n", stats->ndir, stats->ndir * 100.0 / stats->ntot);
    printf("Специальные файлы блочных устройств:    %-7ld %-5.2f %%\n", stats->nblk, stats->nblk * 100.0 / stats->ntot);
    printf("Специальные файлы символьных устройств: %-7ld %-5.2f %%\n", stats->nchr, stats->nchr * 100.0 / stats->ntot);
    printf("FIFO:                                   %-7ld %-5.2f %%\n", stats->nfifo, stats->nfifo * 100.0 / stats->ntot);
    printf("Символические ссылки:                   %-7ld %-5.2f %%\n", stats->nslink, stats->nslink * 100.0 / stats->ntot);
    printf("Сокеты:                                 %-7ld %-5.2f %%\n", stats->nsock, stats->nsock * 100.0 / stats->ntot);
    printf("Всего:                                  %-7ld\n", stats->ntot);
    printf(RESET);

}

static int showFiles(const char *pathname, int type, ino_t i)
{
    switch (type)
    {
    case FTW_NS:
        printf("ERROR: mistake in call func lstat for %s\n", pathname);
        break;
    case FTW_F:
        printf(" %s (%d)\n", pathname, (int)(i));
        break;
    case FTW_D:
        printf(" %s (%d)/\n", pathname, (int)(i));
        break;
    case FTW_DNR:
        printf("ERROR: access to catalog %s is closed\n", pathname);
        break;
    
    default:
        printf("ERROR: unexpected type %d of file %s\n", type, pathname);
        break;
    }

    return 0;
}


static int dopath(const char *filename, int n) 
{
    struct stat     statbuf;    
    struct dirent   *dirp;      
    DIR             *dp;         
    int             ret = 0;    


    if (lstat(filename, &statbuf) < 0)
    {
        return(showFiles(filename, FTW_NS, statbuf.st_ino));
    }

    for (int i = 0; i < n; i++)
        printf("______");

    if (S_ISDIR(statbuf.st_mode) == 0)  
    {
        incStats(statbuf.st_mode, &stats);

        return(showFiles(filename, FTW_F, statbuf.st_ino));
    }

    stats.ndir++;
    showFiles(filename, FTW_D, statbuf.st_ino);

    if ((dp = opendir(filename)) == NULL)
        return(showFiles(filename, FTW_DNR, statbuf.st_ino));

    chdir(filename);

    while ((dirp = readdir(dp)) != NULL && (ret == 0))
    {
        if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))       
            ret = dopath(dirp->d_name, n + 1);
    }

    chdir("..");

    if (closedir(dp) < 0) 
        printf("ERROR: imposiible to close catalog %s", filename);

    return(ret);
}


int main(int argc, char *argv[])
{
    int ret;

    if (argc != 2)
    {
        printf("ERROR: wrong number of arguments (must be one - starting dir)\n");
        exit(1);
    }

    ret = dopath(argv[1], 0);

    printStats(&stats);
    
    return ret;
}