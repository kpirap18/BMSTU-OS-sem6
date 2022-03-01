// Inode - это структура данных в которой хранится информация о файле или 
// директории в файловой системе. В файловой системе Linux, например Ext4, 
// у файла есть не только само его содержимое, например, тот текст, 
// но и метаданные, такие как имя, дата создания, доступа, модификации и права. 
// Вот эти метаданные и хранятся в Inode. У каждого файла есть своя уникальная 
// Inode и именно здесь указано в каких блоках находятся данные файла.

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
/*
struct stat {
    dev_t         st_dev;      // устройство 
    ino_t         st_ino;      // inode 
    mode_t        st_mode;     // режим доступа 
    nlink_t       st_nlink;    // количество жестких ссылок 
    uid_t         st_uid;      // идентификатор пользователя-владельца 
    gid_t         st_gid;      // идентификатор группы-владельца 
    dev_t         st_rdev;     // тип устройства 
                               // (если это устройство) 
    off_t         st_size;     // общий размер в байтах 
    blksize_t     st_blksize;  // размер блока ввода-вывода 
                               // в файловой системе 
    blkcnt_t      st_blocks;   // количество выделенных блоков 
    time_t        st_atime;    // время последнего доступа 
    time_t        st_mtime;    // время последней модификации 
    time_t        st_ctime;    // время последнего изменения 
};
*/
/*
В POSIX.1 определены только поля d_name и d_ino. 
 Остальные поля доступны во многих, но не во всех системах. 
struct dirent {
    ino_t          d_ino;       // номер inode 
    off_t          d_off;       // не смещение; смотрите ЗАМЕЧАНИЯ 
    unsigned short d_reclen;    // длина этой записи 
    unsigned char  d_type;      // тип файла; поддерживается
                                   не всеми типами файловых систем 
    char           d_name[256]; // имя файла 
};
*/

static statistics stats; // данная структура определена в stats.h (мой файл)

// Обход дерева каталогов
int doPath(myfunc *func, char *fullpath, int depth)
{
    if (depth < 0) 
    {
        chdir(fullpath);
        return SUCCESS; // ВОЗВРАТ из просмотренного каталога
    }

    struct stat statbuf;
    struct dirent *dirp;
    // Тип данных DIR представляет поток каталога.
	DIR *dp;
	
	// stat возвращает информацию о файле file_name и заполняет буфер buf. 
	// lstat идентична stat, но в случае символьных ссылок она возвращает 
	// информацию о самой ссылке, а не о файле, на который она указывает.
    if (lstat(fullpath, &statbuf) == -1)
        return ERROR; // ВОЗВРАТ ошибка получения статистики

    if (!S_ISDIR(statbuf.st_mode)) // файловый режим
    {
        // Это не каталог
        incStats(&statbuf, &stats); // подсчет статистики по файлам
        func(fullpath, FTW_F, depth); // для отрисовки

        return SUCCESS; // ВОЗВРАТ если это не каталог 
    }

    // если не файл, а каталог, то ++ каталоги
    func(fullpath, FTW_D, depth); // отрисовка
    stats.ndir++;

	// Opendir функция открывает и возвращает поток каталога для 
	// чтения каталога, чье имя dirname. Поток имеет тип DIR *.
    if ((dp = opendir(fullpath)) == NULL) 
        return ERROR; // ВОЗВРАТ каталог не доступен

    // Переходим в директорию
    if (chdir(fullpath) == -1)
    {
        closedir(dp);
        return ERROR; // ВОЗВРАТ в случае неудачи при переходе в каталог
    }

    depth++; // глубина ++ (для отрисовки)

    // Элемент возврата
	// когда мы обработаем каталог, то мы должны выйти из него
    struct stackItem item = {.fileName = "..", .depth = -1};
    push(&stk, &item);

	// Затем в цикле вызывается функция readdir, которая читает очередную 
	// запись и возвращает указатель на структуру dirent или пустой указатель,
	// если все записи были прочитаны. Все, что нам сейчас нужно в структуре 
	// dirent, — это имя файла (d_name). Это имя можно передать функции stat 
	// (раздел 4.2), чтобы определить атрибуты файла.
	
	//  Если readdir() возвращает NULL и errno равняется 0 (или, по-другому, 
	// EOK или ENOERROR), это значит, что в каталоге больше нет записей.

    while ((dirp = readdir(dp)) != NULL)
    {
        // Пропуск . и .. // текущий и родительский каталог
        if (strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0)
        {
            strcpy(item.fileName, dirp->d_name);
            item.depth = depth;
            push(&stk, &item);
        }
    }
	
	if (errno == 0)
	{
		printf("В каталоге больше нет записей\n");
	}

    if (closedir(dp) == -1)
        return ERROR; // ВОЗВРАТ в случае ошибки при закрытии каталога

    return SUCCESS;
}

// Первичный вызов для переданного программе каталога
static int myFtw(char *pathname, myfunc *func)
{
    // Меняем текущую директорию на переданную
    if (chdir(pathname) == -1)
    {
        printf("%sПереданного каталога не существует, либо он не доступен%s", RED, RESET);
        return ERROR; // ВОЗВРАТ ошибка переключения на исследуемый каталог
    }

    init(&stk);

    struct stackItem item = {.depth = 0};
	// #define PATH_MAX        4096    /* # chars in a path name including nul */
    char cwd[PATH_MAX];
	
    // getcwd копирует абсолютный путь к текущему рабочему каталогу в массиве, 
    // на который указывает buf (1ый аргумент), имеющий длину size.
    // NULL при ошибках (например, текущий каталог считывать невозможно)
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("%s Ошибка: невозможно получить путь рабочего каталога %s", RED, RESET);
        return ERROR; // ВОЗВРАТ ошибка получения пути рабочего каталога
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

// эту функцию вызываю везде как func
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
        return 1; // ВОЗВРАТ не передан начальный каталог
    }

    return myFtw(argv[1], myFunc);
}
