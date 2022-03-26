#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    uint64_t pfn : 55;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
} PagemapEntry;

#define BUFFSIZE 10000

// pidof Telegram, для примера
#define PID 2980
FILE *f = NULL;

void printCMDLINE()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/cmdline", PID);
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    int len = fread(buf, 1, BUFFSIZE, file);
    buf[len - 1] = 0;

    fprintf(f, "\nCMDLINE CONTENT (полная командная строка процесса, если он не зомби):\n");
    fprintf(f, "%s\n", buf);

    fclose(file);
}

void printENVIRON()
{

    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/environ", PID);
    FILE *file = fopen(pathToOpen, "r");

    int len;
    char buf[BUFFSIZE];
    fprintf(f, "\nENVIRON (файл окружения, исходная среда, которая была установлена при запуске выполняемой программы):\n");
    while ((len = fread(buf, 1, BUFFSIZE, file)) > 0)
    {
        for (int i = 0; i < len; i++)
            if (!buf[i])
                buf[i] = '\n';
        buf[len - 1] = '\n';
        fprintf(f, "%s", buf);
    }

    fclose(file);
}

void printFD()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/fd/", PID);
    DIR *dir = opendir(pathToOpen);
    

    fprintf(f, "\nFD(поддиректория, которая содержит одну запись (символическая ссылка на файл) для файла, открытого "
           "процессом):\n");

    struct dirent *readDir;
    char string[PATH_MAX];
    char path[BUFFSIZE] = {'\0'};
    while ((readDir = readdir(dir)) != NULL)
    {
        if ((strcmp(readDir->d_name, ".") != 0) && (strcmp(readDir->d_name, "..") != 0))
        {
            sprintf(path, "%s%s", pathToOpen, readDir->d_name);
            readlink(path, string, PATH_MAX);
            fprintf(f, "{%s} -- %s\n", readDir->d_name, string);
        }
    }

    closedir(dir);
}

void printSTAT()
{
    char *statMeanings[] = {
        "pid", "ID процесса",
        "comm", "Имя файла",
        "state", "Состояние процесса",
        "ppid", "ID родительского процесса",
        "pgrp", "ID группы процесса",
        "session", "ID сессии процесса",
        "tty_nr", "управляющий терминал процесса",
        "tpgid", "ID внешней группы процессов управляющего терминала",
        "flags", "Флаги ядра процесса",
        "minflt", "Количество минорных ошибок процесса (Минорные ошибки не включают ошибки загрузки страниц памяти с "
        "диска)",
        "cminflt", "Количество минорных ошибок дочерних процессов (Минорные ошибки не включают ошибки загрузки страниц "
        "памяти с диска)",
        "majflt", "Количество Мажоных ошибок процесса",
        "cmajflt", "Количество Мажоных ошибок дочерних процессов процесса",
        "utime", "Количество времени, в течение которого этот процесс исполнялся в пользовательском режиме",
        "stime", "Количество времени, в течение которого этот процесс исполнялся в режиме ядра",
        "cutime", "Количество времени, в течение которого дети этого процесса исполнялись в "
        "пользовательском режиме",
        "cstime", "Количество времени, в течение которого дети этого процесса были запланированы в режиме "
        "ядра",
        "priority", "Приоритет процесса",
        "nice", "уровень nice процесса, то есть пользовательский приоритет процесса (-20 - высокий, 19 - низкий)",
        "num_threads", "Количество потоков",
        "itrealvalue", "Время в тиках до следующего SIGALRM отправленного в процесс из-за интервального таймера",
        "starttime", "Время, прошедшее с момента загрузки системы, когда процесс был запущен",
        "vsize", "Объем виртуальной памяти в байтах",
        "rss", "Resident Set (Memory) Size: Количество страниц процесса в физической памяти",
        "rsslim", "Текущий предел в байтах для RSS процесса",
        "startcode", "Адрес, над которым может работать текст программы",
        "endcode", "Адрес, до которого может работать текст программы",
        "startstack", "Адрес начала стека",
        "kstkesp", "Текущее значение ESP (Stack pointer)",
        "kstkeip", "Текущее значение EIP (instruction pointer)",
        "pending", "Битовая карта отложенных сигналов, отображаемое в виде десятичного числа",
        "blocked", "Битовая карта заблокированных сигналов, отображаемое в виде десятичного числа",
        "sigign", "Битовая карта игнорированных сигналов, отображаемое в виде десятичного числа",
        "sigcatch", "Битовая карта пойманных сигналов, отображаемое в виде десятичного числа",
        "wchan", "Адрес ядрёной функции, где процесс находится в состоянии сна",
        "nswap", "Количество страниц, поменявшихся местами",
        "cnswap", "Накопительный своп для дочерних процессов",
        "exit_signal", "Сигнал, который будет послан родителю, когда процесс будет завершен",
        "processor", "Номер процессора, на котором происходило последнее выполнение",
        "rt_priority", "Приоритет планирования в реальном времени - число в диапазоне от 1 до 99 для процессов, "
        "запланированных в соответствии с политикой реального времени",
        "policy", "Политика планирования",
        "blkio_ticks", "Общие блочные задержки ввода/вывода",
        "gtime", "Гостевое время процесса",
        "cgtime", "Гостевое время дочерних процессов",
        "start_data", "Адрес, над которым размещаются инициализированные и неинициализированные данные программы (BSS)",
        "end_data", "Адрес, под которым размещаются инициализированные и неинициализированные данные программы (BSS)",
        "start_brk", "Адрес, выше которого куча программ может быть расширена с помощью brk (станавливает конец "
        "сегмента данных в значение, указанное в аргументе end_data_segment, когда это значение является приемлимым, "
        "система симулирует нехватку памяти и процесс не достигает своего максимально возможного размера сегмента "
        "данных)",
        "arg_start", "Адрес, над которым размещаются аргументы командной строки программы (argv)",
        "arg_end", "Адрес, под которым размещаются аргументы командной строки программы (argv)",
        "env_start", "Адрес, над которым размещена программная среда",
        "env_end", "Адрес, под которым размещена программная среда",
        "exit_code", "Состояние выхода потока в форме, сообщаемой waitpid"};

    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/stat", PID);
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nSTAT (информация о состоянии процесса): \n");

    int d = 0;
    for (int i = 0; token != NULL; i += 2)
    {
        fprintf(f, "%3d %s - %s -- %s\n", d, statMeanings[i], token, statMeanings[i + 1]);
        token = strtok(NULL, " ");
        d++;
    }

    fclose(file);
}

void printSTATM()
{
    char *statmMeanings[] = {"size", "общее число страниц выделенное процессу в виртуальной памяти",
                             "resident", "количество страниц, загруженных в физическую память",
                             "shared", "количество общих резедентных страниц",
                             "trs", "количество страниц кода",
                             "lrs", "количество страниц библиотеки",
                             "drs", "количество страниц данных/стека",
                             "dt", "dirty pages - данные для записи находятся в кэше страниц, но требуется к "
                             "первоначальной записи на носитель"};

    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/statm", PID);
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nSTATM (информация об использовании памяти, измеряемой в страницах): \n");

    int d = 0;
    for (int i = 0; token != NULL; i += 2)
    {
        fprintf(f, "%3d %s - %s -- %s\n", d, statmMeanings[i], token, statmMeanings[i + 1]);
        token = strtok(NULL, " ");
        d++;
    }

    fclose(file);
}

void printIO()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/io", PID);
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nIO: \n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

// +=============================================================

int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr)
{
    size_t nread;
    ssize_t ret;
    uint64_t data;

    nread = 0;
    while (nread < sizeof(data)) {
        ret = pread(pagemap_fd, ((uint8_t*)&data) + nread, sizeof(data) - nread,
                (vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
        nread += ret;
        if (ret <= 0) {
            return 1;
        }
    }
    entry->pfn = data & (((uint64_t)1 << 55) - 1);
    entry->soft_dirty = (data >> 55) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;
    return 0;
}

int virt_to_phys_user(uintptr_t *paddr, pid_t pid, uintptr_t vaddr)
{
    char pagemap_file[BUFSIZ];
    int pagemap_fd;

    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
    pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        return 1;
    }
    PagemapEntry entry;
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
        return 1;
    }
    close(pagemap_fd);
    *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
    return 0;
}

void printPAGEMAP()
{
    char buffer[BUFSIZ];
    char maps_file[BUFSIZ];
    char pagemap_file[BUFSIZ];
    int maps_fd;
    int offset = 0;
    int pagemap_fd;
    pid_t pid;

    fprintf(f, "addr = Если страница присутствует в ОЗУ (бит 63), то эти биты обеспечивают номер страничного фрейма, Если страница присутствует в свопе (бит 62), затем биты 4–0 задают тип подкачки, а биты 54–5 кодируют смещение подкачки.\npfn \nsoft-dirty \nfile/shared = Страница представляет собой страницу с отображением файлов или общую анонимную страницу \nswapped = Если установлено, страница находится в пространстве подкачк \npresent = Если установлено, страница находится в оперативной памяти.\n\n");

    snprintf(maps_file, sizeof(maps_file), "/proc/%ju/maps", (uintmax_t)PID);
    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)PID);
    maps_fd = open(maps_file, O_RDONLY);
    if (maps_fd < 0) {
        perror("open maps");
        // return EXIT_FAILURE;
    }
    pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        perror("open pagemap");
        // return EXIT_FAILURE;
    }
    fprintf(f, "addr pfn soft-dirty file/shared swapped present library\n");
    for (;;) {
        ssize_t length = read(maps_fd, buffer + offset, sizeof buffer - offset);
        if (length <= 0) break;
        length += offset;
        for (size_t i = offset; i < (size_t)length; i++) {
            uintptr_t low = 0, high = 0;
            if (buffer[i] == '\n' && i) {
                const char *lib_name;
                size_t y;
                {
                    size_t x = i - 1;
                    while (x && buffer[x] != '\n') x--;
                    if (buffer[x] == '\n') x++;
                    while (buffer[x] != '-' && x < sizeof buffer) {
                        char c = buffer[x++];
                        low *= 16;
                        if (c >= '0' && c <= '9') {
                            low += c - '0';
                        } else if (c >= 'a' && c <= 'f') {
                            low += c - 'a' + 10;
                        } else {
                            break;
                        }
                    }
                    while (buffer[x] != '-' && x < sizeof buffer) x++;
                    if (buffer[x] == '-') x++;
                    while (buffer[x] != ' ' && x < sizeof buffer) {
                        char c = buffer[x++];
                        high *= 16;
                        if (c >= '0' && c <= '9') {
                            high += c - '0';
                        } else if (c >= 'a' && c <= 'f') {
                            high += c - 'a' + 10;
                        } else {
                            break;
                        }
                    }
                    lib_name = 0;
                    for (int field = 0; field < 4; field++) {
                        x++;
                        while(buffer[x] != ' ' && x < sizeof buffer) x++;
                    }
                    while (buffer[x] == ' ' && x < sizeof buffer) x++;
                    y = x;
                    while (buffer[y] != '\n' && y < sizeof buffer) y++;
                    buffer[y] = 0;
                    lib_name = buffer + x;
                }
                /* Get info about all pages in this page range with pagemap. */
                {
                    PagemapEntry entry;
                    for (uintptr_t addr = low; addr < high; addr += sysconf(_SC_PAGE_SIZE)) {
                        /* TODO always fails for the last page (vsyscall), why? pread returns 0. */
                        if (!pagemap_get_entry(&entry, pagemap_fd, addr)) {
                            fprintf(f, "%jx %jx %u %u %u %u %s\n",
                                (uintmax_t)addr,
                                (uintmax_t)entry.pfn,
                                entry.soft_dirty,
                                entry.file_page,
                                entry.swapped,
                                entry.present,
                                lib_name
                            );
                        }
                    }
                }
                buffer[y] = '\n';
            }
        }
    }
    close(maps_fd);
    close(pagemap_fd);
    // return EXIT_SUCCESS;
}

// ==============================================================


void printSMAPS()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/smaps", PID);
    FILE *file = fopen(pathToOpen, "r"); 

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nSMAPS: \n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

void printWCHAN()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/wchan", PID);
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nWCHAN Символическое имя, соответствующее местоположению в  ядре, в котором процесс спит. \n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

void printCWD()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/cwd", PID);

    char buf[BUFFSIZE] = {'\0'};

    readlink(pathToOpen, buf, BUFFSIZE);

    fprintf(f, "CWD (символическая ссылка на директорию процесса):\n");
    fprintf(f, "%s\n", buf);
}

void printEXE()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/exe", PID);

    char buf[BUFFSIZE] = {'\0'};

    readlink(pathToOpen, buf, BUFFSIZE);

    fprintf(f, "EXE (символическая ссылка, которая содержит путь к выполненной команде):\n");
    fprintf(f, "%s\n", buf);
}

void printMAPS()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/maps", PID);

    char buf[BUFFSIZE] = {'\0'};
    FILE *file = fopen(pathToOpen, "r");

    fprintf(f, "MAPS (список выделенных учатков памяти, используемых процессом и права доступа)\n"
           "Начальный адрес участка памяти : конечный участок участка памяти"
           "\nПрава доступа(r - доступно для чтения, w - доступно для изменения, x - доступно для выполнения, s - "
           "разделяемый, p - "
           "private (copy on write))"
           "\nсмещение распределения"
           "\nстарший и младший номер устройства"
           "\ninode:\n");
    int lengthOfRead;
    while ((lengthOfRead = fread(buf, 1, BUFFSIZE, file)))
    {
        buf[lengthOfRead] = '\0';
        fprintf(f, "%s\n", buf);
    }

    fclose(file);
}

void printROOT()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/%d/root", PID);

    char buf[BUFFSIZE] = {'\0'};

    readlink(pathToOpen, buf, BUFFSIZE);

    fprintf(f, "ROOT (символическая ссылка, которая указывает на корень файловой системы, которой принадлежит процесс):\n");
    fprintf(f, "%s\n", buf);
}

void printFILESYSTEMS()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/filesystems");
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nFILESYSTEMS Текстовый список файловых систем, которые поддерживаются ядром, а именно файловые системы, которые были скомпилированы \nв ядро или чьи модули ядра загружены в данный момент. (См. также filesystems(5).)  \nЕсли файловая система помечена символом  \"nodev\", это означает, что она не требует блочного устройства для монтирования \n(например, виртуальная файловая система, сетевая файловая система).\n\
              Кстати, этот файл может быть использован командой mount(8), когда не указана файловая система не указана,\n и ему не удалось определить тип файловой системы.  Тогда перебираются файловые системы, \nсодержащиеся в этом файле (за исключением тех, которые помечены символом  \"nodev\").\n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

void printIOPORTS()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/ioports");
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nIOPORTS Это список зарегистрированных в настоящее время портов ввода-вывода \n регионов, которые используются. \n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

void printMEMINFO()
{
    char pathToOpen[PATH_MAX];
    char *statmMeanings[] = {"MemTotal Общий объем используемой оперативной памяти (т.е. физическая оперативная память за вычетом нескольких зарезервированных битов и двоичного кода ядра).",
                             "MemFree Сумма LowFree+HighFree.",
                             "MemAvailable Оценка того, сколько памяти доступно для запуска новых приложений без свопинга. ",
                             "Buffers Относительно временное хранилище для необработанных дисковых блоков, которые не должны быть очень большими (20 МБ или около того).",
                             "Кэш-память в памяти для файлов, считываемых с диска (страничный кэш).  Не включает SwapCached.",
                             "SwapCached Память, которая когда-то была вытеснена, вытеснена обратно  но по-прежнему находится в файле подкачки.",
                             "Активная память, которая использовалась совсем недавно и обычно не освобождается без крайней необходимости.",
                             "Неактивная память, которая использовалась не так давно.  Она более пригодна для использования в других целях.",
                             " ", // Active(anon) 
                             " ", // Inactive(anon)
                             " ", // Active(file) 
                             " ", // Inactive(file) 
                             " ", // Unevictable
                             " ", // Mlocked
                             "SwapTotal Общее количество доступного пространства подкачки.",
                             "SwapFree Количество пространства подкачки, которое в настоящее время не используется",
                             "Грязная память Память, которая ожидает записи обратно на диск.",
                             "Writeback Память, которая активно записывается на диск. диск.",
                             "AnonPages Нефайловые резервные страницы, отображенные в таблицы страниц пространства пользователя. таблицы.",
                             "Mapped Файлы, которые были отображены в память (с помощью функции mmap(2)), например, библиотеки.",
                             "Shmem Объем памяти, потребляемой в файловой системе tmpfs(5).",
                             "KReclaimable Выделения ядра, которые ядро будет пытаться вернуть под давлением памяти.",
                             "Slab Кэш структур данных внутри ядра.",
                             "SReclaimable Часть Slab, которая может быть повторно использована, такая как кэши.",
                             " SUnreclaim Часть Slab, которая не может быть освобождена",
                             "KernelStack Объем памяти, выделенный под стеки ядра.",
                             "PageTables Объем памяти, выделенный для самого низкого уровня  таблицам страниц.",
                             "NFS_Unstable Страницы NFS, отправленные на сервер, но еще не зафиксированные  в стабильное хранилище.",
                             "Bounce Память, используемая для \"буферов отскока\" блочных устройств.",
                             "WritebackTmp Память, используемая FUSE для временных буферов записи буферов.",
                             "CommitLimit Это общий объем памяти, который в настоящее время может быть выделен в системе, выраженный в килобайтах.  ",
                             "Committed_AS Объем памяти, выделенный в настоящее время в системе.  Зафиксированная память - это сумма всей памяти, которая была выделена процессами, даже если она еще не была ими \"использована\". ",
                             "VmallocTotal Общий размер области памяти vmalloc.",
                             "VmallocUsed Количество используемой области vmalloc",
                             "VmallocChunk Самый большой непрерывный блок области vmalloc, который свободен.",
                             " ", // Percpu
                             " ", // HardwareCorrupted
                             " ", // AnonHugePages
                             " ", // ShmemHugePages
                             " ", // ShmemPmdMapped
                             " ", // FileHugePages
                             " ", // FilePmdMapped
                             "HugePages_Total Размер пула огромных страниц.",
                             "HugePages_Free Количество  огромных страниц в пуле, которые еще не выделены.",
                             "HugePages_Rsvd Это количество огромных страниц, для которых было принято обязательство о выделении из пула, но выделение еще не было произведено.  ",
                             "HugePages_Surp Это количество огромных страниц в пуле, превышающее значение в /proc/sys/vm/nr_hugepages. ",
                             "Hugepagesize Размер огромных страниц.",
                             " ", // Hugetlb
                             "DirectMap4k Количество байт оперативной памяти, линейно отображаемых ядром в страницы размером 4 кБ. ",
                             "DirectMap2M Количество байт оперативной памяти, линейно отображаемых ядром на страницы размером 2 МБ.  ",
                             " ",
                             " ",
                             " ",
                             " "};

    snprintf(pathToOpen, PATH_MAX, "/proc/meminfo");
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nMEMINFO: \n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}

void printCPUINFO()
{
    char pathToOpen[PATH_MAX];
    snprintf(pathToOpen, PATH_MAX, "/proc/cpuinfo");
    FILE *file = fopen(pathToOpen, "r");

    char buf[BUFFSIZE];
    fread(buf, 1, BUFFSIZE, file);
    char *token = strtok(buf, " ");

    fprintf(f, "\nCPUINFO\n");

    for (int i = 0; token != NULL; i++)
    {
        fprintf(f, "%s  ", token);
        token = strtok(NULL, " ");
    }

    fclose(file);
}


int main()
{
    f = fopen("result2.txt", "w");

    printCMDLINE();
    fprintf(f, "\n==================================\n");

    printENVIRON();
    fprintf(f, "\n==================================\n");

    printFD();
    fprintf(f, "\n==================================\n");

    printSTAT();
    fprintf(f, "\n==================================\n");

    printSTATM();
    fprintf(f, "\n==================================\n");

    printCWD();
    fprintf(f, "\n==================================\n");

    printEXE();
    fprintf(f, "\n==================================\n");

    printMAPS();
    fprintf(f, "\n==================================\n");

    printROOT();
    fprintf(f, "\n==================================\n");

    printIO();
    fprintf(f, "\n==================================\n");

    printSMAPS();
    fprintf(f, "\n==================================\n");

    printPAGEMAP();
    fprintf(f, "\n==================================\n");

    printWCHAN();
    fprintf(f, "\n==================================\n");

    fprintf(f, "\n\n================================== ОБЩАЯ ИНФОРМАЦИЯ О СИСТЕМЕ ==================================\n");

    printFILESYSTEMS();
    fprintf(f, "\n==================================\n");

    printIOPORTS();
    fprintf(f, "\n==================================\n");

    printCPUINFO();
    fprintf(f, "\n==================================\n");

    printMEMINFO();
    fprintf(f, "\n==================================\n");

    fclose(f);
    return 0;
}