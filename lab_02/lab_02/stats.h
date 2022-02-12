#ifndef STAT_H
#define STAT_H

#define RESET "\033[0m"
#define CYAN "\033[0;36m"
#define PURPLE "\033[0;35m"
#define YELLOW "\033[0;33m"

#include <sys/stat.h>

#include <stdio.h>

typedef struct
{
    long nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;
} statistics;

void printStats(statistics *stats);

void incStats(struct stat *mode, statistics *stats);

#endif // STAT_H
