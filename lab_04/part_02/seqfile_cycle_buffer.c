#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/vmalloc.h>
#include <linux/proc_fs.h> 
#include <linux/seq_file.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");
MODULE_DESCRIPTION("Sequence file Buffer Kernel Module");

#define OK 0

#define FORTUNE_DIRNAME "fortune_dir"
#define FORTUNE_FILENAME "fortune_file"
#define FORTUNE_SYMLINK "fortune_symlink"
#define FORTUNE_PATH FORTUNE_FILENAME

#define MAX_BUF_SIZE PAGE_SIZE

/* базовая  структура, для работы с ФС proc в ядре
    хранит номер inode директории, размер имени,
    типы и права доступа, число линков, указатель
    на структуру file_operations и т.д.*/
static struct proc_dir_entry *fortune_dir;
static struct proc_dir_entry *fortune_file;
static struct proc_dir_entry *fortune_symlink;

static char *buffer = NULL;

static int read_index = 0;
static int write_index = 0;

static int fortune_show(struct seq_file *sfile, void *v)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    if (read_index >= write_index)
    {
        read_index = 0;
    }

//seq_printf стандартная функция, выполняет действия, аналогичные copy_to_user или sprintf.

//int seq_printf(struct seq_file *sfile, const char *fmt, ...);
// Это эквивалент printf для реализаций seq_file; он принимает обычную строку формата и дополнительные аргументы значений. 
// Однако, вы также должны передать ей структуру seq_file, которая передаётся в функцию show. 
// Если seq_printf возвращает ненулевое значение, это означает, что буфер заполнен и вывод будет отброшен.
// Большинство реализаций, однако, игнорирует возвращаемое значение.

    seq_printf(sfile, buffer + read_index);

    int len = strlen(buffer + read_index);
    if (len > 0)
    {
        read_index += len + 1;
    }
    
    return OK;
}

static int fortune_open(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
	// чтобы создать один файловый экземпляр модуля используется
    // single_open который передаёт адрес функции my_show, 
    // а функция my_show передаёт адрес страницы памяти
    return single_open(sp_file, fortune_show, NULL);
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
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);;

    // достаточно ли места для размещения фортунки
    if (len > MAX_COOKIE_BUF_SIZE - cookie_index + 1)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Buffer overflow");
        return -ENOSPC;
    }

/* Копирование буфера из пользовательского пространства в пространство ядра */
// 1ый аргумент - куда в пространстве ядра, 
// 2ой - откуда из пространства пользователя,
// 3ий - сколько байт
    if (copy_from_user(&cookie_buffer[cookie_index], buf, len) != 0)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "copy_from_user function get a error");
        return -EFAULT;
    }

    cookie_index += len;
    cookie_buffer[cookie_index - 1] = '\n';

    cookie_buffer[cookie_index] = 0;
    cookie_index += 1;

    return len; // количество символов фактически записанных в cookie_buffer
}
static const struct proc_ops fops =
{
    proc_read: seq_read,
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
        remove_proc_entry(FORTUNE_FILENAME, fortune_dir);
    }

    if (fortune_dir != NULL) 
    {
        remove_proc_entry(FORTUNE_DIRNAME, NULL);
    }

    vfree(buffer);
}

static int __init fortune_init(void) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    // Выделение 'виртуально' непрерывного блока памяти 
    if ((cookie_buffer = vmalloc(MAX_COOKIE_BUF_SIZE)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Allocate memory error.");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, MAX_COOKIE_BUF_SIZE);

    if ((fortune_dir = proc_mkdir(FORTUNE_DIRNAME, NULL)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Error during create directory in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    if ((fortune_file = proc_create(FORTUNE_FILENAME, 0666, NULL, &fops)) == NULL) 
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Error during create file in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    if ((fortune_symlink = proc_symlink(FORTUNE_SYMLINK, NULL, FORTUNE_PATH)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Error during create symlink in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    printk(KERN_INFO "FORTUNE_MODULE: %s.\n", "Module has benn successfully loaded.\n");
    return OK;
}

static void __exit fortune_exit(void) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    cleanup_fortune();
    printk(KERN_INFO "FORTUNE_MODULE: %s.\n", "Module has been successfully removed");
}

module_init(fortune_init);
module_exit(fortune_exit);

// для release и open нужен inode так как там надо работать
// открывать закрывать файлы, а read и write работают уже
// с открытыми.
