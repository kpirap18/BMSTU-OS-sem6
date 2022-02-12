#include "apue.h"

#include <stdarg.h> // va_start, va_end

/*
 * Выводит сообщение и возвращает управление в вызывающую функцию.
 * Вызывающая функция определяет значение флага "errnoflag".
 */
// Раго. 1021 страница.
static void err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char buf[MAXLINE];

	vsnprintf(buf, MAXLINE - 1, fmt, ap);
	if (errnoflag)
		snprintf(buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s",
				 strerror(error));
	strcat(buf, "\n");
	fflush(stdout); /* в случае, когда stdout и stderr - одно и то же устройство */
	fputs(buf, stderr);
	fflush(NULL); /* сбрасывает все выходные потоки */
}

/*
* Обрабатывает фатальные ошибки, не связанные с системными вызовами.
* Выводит сообщение и завершает работу процесса.
*/
// Раго. 1021 страница.
void err_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
* Об­ра­бот­ка фа­таль­ных оши­бок, не свя­зан­ных с сис­тем­ны­ми вы­зо­ва­ми.
* Код ошиб­ки пе­ре­да­ет­ся в ви­де ар­гу­мен­та.
* Вы­во­дит со­об­ще­ние и за­вер­ша­ет ра­бо­ту про­цес­са.
*/
void err_exit(int error, const char *fmt, ...)
{
	va_list
		ap;
	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}