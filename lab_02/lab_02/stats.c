#include "stats.h"

// Приведение статистики
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

// Инкрементация статистических данных
void incStats(struct stat *mode, statistics *stats)
{
    switch (mode->st_mode & S_IFMT)
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
