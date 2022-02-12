// Запускаем два терминала, в одном папка с программой:
// gcc lab_01.c -pthread -Wall -Werror -o main.exe
// sudo ./main.exe
// ps -ajx | grep "./main"

// Во втором терминале открываем cd /var/log
// sudo cat syslog

// По ps: 
// -Ssl
// S - процесс находится в режиме прерываемого сна
// s - процесс является лидером сессии
// l - процесс является многопоточным
// tty - имя терминала, отвечающего за процесс
// tpgid - айди группы процессов терминала, с которым связан процесс. Если -1, то процесс не связан с терминалом
// uid - user id

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include "sys/file.h"
#include <pthread.h>
#include "signal.h"

#define LOCKFILE "/var/run/my_daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

sigset_t mask;

// стр. 560
int lockfile(int fd)
{
	// Эта структура используется для управления блокировкой и имеет следующее содержание:
	// struct flock
	// {
	// 	short l_type; /*3 режима блокирования
	//                F_RDLCK(Разделение чтения)
	//                F_WRLCK (Разделение записи)
	//                F_UNLCK (Прекратить разделение)*/

	// 	off_t l_start; /*относительное смещение в байтах,
	//               зависит от l_whence*/

	// 	short l_whence; /*SEEK_SET;SEEK_CUR;SEEK_END*/

	// 	off_t l_len; /*длина, 0=разделение до конца файла*/

	// 	pid_t l_pid; /*идентификатор, возвращается F_GETLK */
	// };

	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;			
	return (fcntl(fd, F_SETLK, &fl));
}

int alreadyRunning(void) 
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) 
	{
        syslog(LOG_ERR, "не возможно открыть %s: %s", LOCKFILE,
               strerror(errno));
        exit(1);
    }

	if (lockfile(fd) < 0)
	{
		if (errno == EACCES || errno == EAGAIN)
		{
			close(fd);
			return (1);
		}
		syslog(LOG_ERR, "невозможно установить блокировку на %s: %s", LOCKFILE, strerror(errno));
		exit(1);
	}

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}

void quit(const char *errMessage, const char *command) 
{
    syslog(LOG_ERR, "%s %s", errMessage, command);
    exit(1);
}

void *threadFun(void *arg) 
{
    int err, signo;

    for (;;) {
        // syslog(LOG_INFO, "Поток для сигналов: %ld", pthread_self());
        err = sigwait(&mask, &signo);
        if (err != 0)
            quit("ошибка вызова функции sigwait", "");
        switch (signo) {
            case SIGHUP:
                syslog(LOG_INFO, "получен сигнал SIGHUP; getlogin=%s", getlogin());
                break;
            case SIGTERM:
                syslog(LOG_INFO, "получен сигнал SIGTERM; выход");
                exit(0);
            default:
                syslog(LOG_INFO, "получен непредвиденный сигнал %d\n", signo);
        }
    }
    return 0;
}

void daemonize(const char *cmd) {
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    
    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
        quit("%s: невозможно получить максимальный номер дескриптора ", cmd);
	}

    if ((pid = fork()) < 0)
	{
        quit("%s: ошибка вызова функции fork", cmd);
	}
    else if (pid != 0) 
	{
        exit(0);
	}
    setsid();

    // Убираем возможность обретения управляющего терминала
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

	
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        quit("%s: невозможно игнорировать сигнал SIGHUP ", cmd);

    if (chdir("/") < 0)
        quit("%s: невозможно сделать текущим рабочим каталогом /", cmd);
printf("2\n");
    if (rl.rlim_max == RLIM_INFINITY) 
		rl.rlim_max = 1024;
	printf("3\n");
    for (i = 0; i < rl.rlim_max; i++) 
		close(i);
printf("4\n");
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
printf("5\n");
    // Инициализировать файл журнала
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) 
	{
		printf("6\n");
        syslog(LOG_ERR, "ошибочные файловые дескрипторы %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
	printf("7\n");
}

int main() 
{
	setbuf(stdout, NULL);
	printf("1\n");
    daemonize("my_daemon");

    if (alreadyRunning())
	{
		quit("демон уже запущен", "");
	}

    int err;
	pthread_t tid;
    struct sigaction sa;

    /*
     * Восстановить действие по умолчанию для сигнала SIGHUP
     * и заблокировать все сигналы
    */
    sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
        quit("невозможно восстановить действие SIG_DFL для SIGHUP", "");

	sigfillset(&mask);
	if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        quit("ошибка выполнения операции SIG_BLOCK", "");

    /*
     *  Создание потока, который будет заниматься обработкой SIGHUP и SIGTERM
     */
    err = pthread_create(&tid, NULL, threadFun, 0);
	if (err != 0)
        quit("невозможно создать поток", "");

// Эксперимент!!!

    int signo;

    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0)
            quit("ошибка вызова функции sigwait", "");
        switch (signo) {
            case SIGHUP:
                syslog(LOG_INFO, "Вызов getlogin. Результат: %s", getlogin());
                break;
            case SIGTERM:
                syslog(LOG_INFO, "получен сигнал SIGTERM; выход");
                exit(0);
            default:
                syslog(LOG_INFO, "получен непредвиденный сигнал %d\n", signo);
        }
    }


    while (1) {
        syslog(LOG_INFO, "Идентификатор созданного потока для обработки сигнала: %ld", tid);
        sleep(5);
    }
}