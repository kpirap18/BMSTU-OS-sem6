#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/vmalloc.h>
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");
MODULE_DESCRIPTION("Fortune Cookie Kernel Module");

#define OK 0

#define FORTUNE_DIRNAME "fortune_dir"
#define FORTUNE_FILENAME "fortune_file"
#define FORTUNE_SYMLINK "fortune_symlink"
#define FORTUNE_PATH FORTUNE_FILENAME 

#define MAX_COOKIE_BUF_SIZE PAGE_SIZE


/* базовая  структура, для работы с ФС proc в ядре
    хранит номер inode директории, размер имени,
    типы и права доступа, число линков, указатель
    на структуру file_operations и т.д.*/
static struct proc_dir_entry *fortune_dir;
static struct proc_dir_entry *fortune_file;
static struct proc_dir_entry *fortune_symlink;

static char *cookie_buffer = NULL; // хранилище фортунок

static int read_index = 0;
static int write_index = 0;

char tmp_buffer[MAX_COOKIE_BUF_SIZE];

static int fortune_open(struct inode *sp_inode, struct file *sp_file) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return OK;
}

static int fortune_release(struct inode *sp_node, struct file *sp_file) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return OK;
}

// пишем в ядро
// 1ый параметр - указатель на структуру file, 
// buf - указывает на буфер данных чтения
// count определяет размер передаваемых данных, 
// f_pos указывает на смещение от начала данных файла для операции чтения
static ssize_t fortune_write(struct file *file, const char __user *buf, size_t len, loff_t *f_pos) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    // достаточно ли места для размещения фортунки
    if (len > MAX_COOKIE_BUF_SIZE - write_index + 1)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Buffer overflow");
        return -ENOSPC;
    }

/* Копирование буфера из пользовательского пространства в пространство ядра */
// 1ый аргумент - куда в пространстве ядра, 
// 2ой - откуда из пространства пользователя,
// 3ий - сколько байт
    if (copy_from_user(&cookie_buffer[write_index], buf, len) != 0)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "copy_from_user function get a error");
        return -EFAULT;
    }

    write_index += len;
    cookie_buffer[write_index - 1] = '\n';

    cookie_buffer[write_index] = '\0';
    write_index += 1;

    return len; // количество символов фактически записанных в cookie_buffer
}

// читаем из ядра
// 1ый параметр - указатель на структуру file, 
// buf - указывает на буфер данных записи
// count определяет размер передаваемых данных, 
// f_pos указывает на смещение от начала данных файла для операций записи
// (f_pos: текущая позиция чтения/записи в файле)
// Тип loff_t образован от сочетания “long_offset” (длинное смещение) имеет не меньше 64 бит емкости даже на 32-битовых платформах. В случае ошибки возвращается отрицательный код ошибки. Функция должна изменять положение счетчика в структуре file, которая описывается в этой главе позднее. Если функция не определена, ядро возвращает ошибку end-of-file (конец файла)
static ssize_t fortune_read(struct file *file, char __user *buf, size_t len, loff_t *f_pos) 
{
    int read_len;
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    // почему смещение не должно быть больше 0
    if (*f_pos > 0 || write_index == 0)
    {
        return 0;
    }

    // если дошли до места, куда только собираемся что-то писать
    if (read_index >= write_index)
    {
        // для зацикливания буфера
        read_index = 0;
    }

// 1ый Адрес назначения находится в пространстве пользователя.
// 2ой Адрес источника находится в пространстве ядра.
// 3ий количество копируемых байт.
// Функция возвращает количество байт, которые не могут быть скопированы.
// В случае успешного выполнения будет возвращен 0.
    read_len = snprintf(tmp_buffer, MAX_COOKIE_BUF_SIZE, "%s\n", &cookie_buffer[read_index]);
    if (copy_to_user(buf, tmp_buffer, read_len) != 0)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "copy_to_user function get a error");
        return -EFAULT;
    }

    read_index += read_len;
    *f_pos += read_len;

    return read_len;
}

static const struct proc_ops fops =
{
    proc_read: fortune_read,
    proc_write: fortune_write,
    proc_open: fortune_open,
    proc_release: fortune_release,
};

static void cleanup_fortune(void)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    if (fortune_symlink != NULL) 
    {
        remove_proc_entry(FORTUNE_SYMLINK, NULL);
    }

    if (fortune_file != NULL)
    {
        remove_proc_entry(FORTUNE_FILENAME, NULL);

    }

    if (fortune_dir != NULL) 
    {
        remove_proc_entry(FORTUNE_DIRNAME, NULL);
    }

    vfree(cookie_buffer);
}

static int __init fortune_init(void) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);;

    // Выделение 'виртуально' непрерывного блока памяти 
    if ((cookie_buffer = vmalloc(MAX_COOKIE_BUF_SIZE)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Allocate memory error.");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, MAX_COOKIE_BUF_SIZE);

    // имя самой директории, ук-ль на proc_dir_entry (корневой каталог)
    if ((fortune_dir = proc_mkdir(FORTUNE_DIRNAME, NULL)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Error during create directory in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    // создаем файл = регистрируем структуру
    // имя файла, права доступа, указатель на родительскую директорию 
    // (у нас NULL - корневой каталог /proc),
    // ук-ль на определ. нами операции на файлах (на file_operations)
    // 0666 -- всем можно читать и писать
    if ((fortune_file = proc_create(FORTUNE_FILENAME, 0666, NULL, &fops)) == NULL) 
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n","Error during create file in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    //имя самой ссылки, ук-ль на proc_dir_entry (корневой каталог), имя файла на который она указывает
    if ((fortune_symlink = proc_symlink(FORTUNE_SYMLINK, NULL, FORTUNE_PATH)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n","Error during create symlink in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    if (!fortune_dir || !fortune_symlink)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n","Error symlink and file");
        cleanup_fortune();
        return -ENOMEM;
    }

    write_index = 0;
    read_index = 0;

    printk(KERN_INFO "++ MY_FORTUNE: %s.\n", "Module has benn successfully loaded.\n");
    return OK;
}

static void __exit fortune_exit(void) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);;
    cleanup_fortune();
    printk(KERN_INFO "++ MY_FORTUNE: %s.\n", "Module has been successfully removed");
}

module_init(fortune_init);
module_exit(fortune_exit);
