/*Для очень простых виртуальных файлов есть еще более простой интерфейс. 
Модуль может определить только функцию show(), которая должна создавать 
весь вывод, который будет содержать виртуальный файл.
 Затем метод open() файла вызывает:

int single_open(struct file *file,
                int (*show)(struct seq_file *m, void *p),
                void *data);
Когда придет время вывода, функция show() будет вызвана один раз. 
Значение данных, переданное single_open(), можно найти в частном поле 
структуры seq_file. При использовании single_open() программист 
должен использовать single_release() вместо seq_release() 
в структуре file_operations, чтобы избежать утечки памяти.
*/
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/vmalloc.h>
#include <linux/proc_fs.h> 
#include <linux/seq_file.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");
MODULE_DESCRIPTION("Fortune Cookie Kernel Module");

#define OK 0


#define FORTUNE_DIRNAME "fortune_dir"
#define FORTUNE_FILENAME "fortune_file"
#define FORTUNE_SYMLINK "fortune_symlink"
#define FORTUNE_PATH FORTUNE_DIRNAME "/" FORTUNE_FILENAME

#define MAX_COOKIE_BUF_SIZE PAGE_SIZE


/* базовая  структура, для работы с ФС proc в ядре
    хранит номер inode директории, размер имени,
    типы и права доступа, число линков, указатель
    на структуру file_operations и т.д.*/
static struct proc_dir_entry *fortune_dir;
static struct proc_dir_entry *fortune_file;
static struct proc_dir_entry *fortune_symlink;

static char *buffer = NULL;

static int fortune_show(struct seq_file *sfile, void *v)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    seq_printf(sfile, buffer);
    
    return OK;
}

static int fortune_open(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return single_open(sp_file, fortune_show, NULL);
}

static int fortune_release(struct inode *sp_node, struct file *sp_file) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return single_release(sp_node, sp_file);
}

static ssize_t fortune_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    if (len > MAX_COOKIE_BUF_SIZE)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Buffer overflow");
        return -ENOSPC;
    }

    if (copy_from_user(buffer, buf, len) != 0)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "copy_from_user function get a error");
        return -EFAULT;
    }

    buffer[len - 1] = '\0';

    return len;
}

static const struct proc_ops fops =
{
    proc_write: fortune_write,
    proc_read: seq_read,
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

    if ((buffer = vzalloc(MAX_COOKIE_BUF_SIZE)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Allocate memory error");
        return -ENOMEM;
    }

    if ((fortune_dir = proc_mkdir(FORTUNE_DIRNAME, NULL)) == NULL)
    {
        printk(KERN_ERR "++ MY_FORTUNE: %s.\n", "Error during create directory in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    int data = 1;
    if ((fortune_file = proc_create_data(FORTUNE_FILENAME, 0666, fortune_dir, &fops, &data)) == NULL) 
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

    printk(KERN_INFO "++ MY_FORTUNE: %s.\n", "Module has benn successfully loaded.\n");
    return OK;
}

static void __exit fortune_exit(void) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    cleanup_fortune();
    printk(KERN_INFO "++ MY_FORTUNE: %s.\n", "Module has been successfully removed");
}

module_init(fortune_init);
module_exit(fortune_exit);