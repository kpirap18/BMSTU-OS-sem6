#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "stack.h"
#include "stats.h"

#define RESET "\033[0m"
#define RED "\033[0;31m"

#define FTW_F 1 // Файл, не являющийся каталогом
#define FTW_D 2 // Каталог

#define ERROR 1
#define SUCCESS 0

typedef int myfunc(const char *, int, int);

static struct stack stk;

static statistics stats;

// Обход дерева каталогов
int doPath(myfunc *func, char *fullpath, int depth)
{
    if (depth < 0) // Возврат из просмотренного каталога
    {
        chdir(fullpath);
        return SUCCESS;
    }

    struct stat statbuf;
    struct dirent *dirp;
    DIR *dp;

    // возвращают информацию об указанном файле. 
    if (lstat(fullpath, &statbuf) == -1)
        return ERROR;

    if (!S_ISDIR(statbuf.st_mode)) // файловый режим
    {
        // Это не каталог
        incStats(&statbuf, &stats); // подсчет статистики по файлам
        func(fullpath, FTW_F, depth); // для отрисовки

        return SUCCESS;
    }

    // если не файл, а каталог, то ++ каталоги
    func(fullpath, FTW_D, depth);
    stats.ndir++;

    // Каталог не доступен 
    if ((dp = opendir(fullpath)) == NULL) 
        return ERROR;

    // Переходим в директорию
    if (chdir(fullpath) == -1)
    {
        closedir(dp);
        return ERROR;
    }

    depth++; // глубина ++

    // Элемент возврата
    struct stackItem item = {.fileName = "..", .depth = -1};
    push(&stk, &item);

    while ((dirp = readdir(dp)) != NULL)
    {
        // Пропуск . и ..
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
        {
            strcpy(item.fileName, dirp->d_name);
            item.depth = depth;
            push(&stk, &item);
        }
    }

    if (closedir(dp) == -1)
        return ERROR;

    return SUCCESS;
}

// Первичный вызов для переданного программе каталога
static int myFtw(char *pathname, myfunc *func)
{
    // Меняем текущую директорию на переданную
    if (chdir(pathname) == -1)
    {
        printf("%sПереданного каталога не существует, либо он не доступен%s", RED, RESET);
        return ERROR;
    }

    init(&stk);

    struct stackItem item = {.depth = 0};
    char cwd[PATH_MAX];
    // копирует абсолютный путь к текущему рабочему каталогу в массиве, 
    // на который указывает buf, имеющий длину size.
    // NULL при ошибках (например, текущий каталог считывать невозможно)
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("%s Ошибка: невозможно получить путь рабочего каталога %s", RED, RESET);
        return ERROR;
    }

    strcpy(item.fileName, cwd);
    push(&stk, &item);

    while (!empty(&stk))
    {
        doPath(func, item.fileName, item.depth);
        item = pop(&stk);
    }

    printStats(&stats);

    return SUCCESS;
}

static int myFunc(const char *pathname, int type, int depth)
{
    for (int i = 0; i < depth; i++)
        printf("    │");


    printf("    ├── %s%s\n", pathname, RESET);

    return SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("%sИспользование: %s <начальный_каталог>%s\n", RED, argv[0], RESET);
        return 1;
    }

    return myFtw(argv[1], myFunc);
}
