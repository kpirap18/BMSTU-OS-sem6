#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/vmalloc.h>
#include <linux/proc_fs.h> 
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");
MODULE_DESCRIPTION("Fortune Cookie Kernel Module");

#define OK 0

#define FORTUNE_DIRNAME "fortune_dir"
#define FORTUNE_FILENAME "fortune_file"
#define FORTUNE_SYMLINK "fortune_symlink"
#define FORTUNE_PATH FORTUNE_DIRNAME "/" FORTUNE_FILENAME

#define MAX_BUF_SIZE PAGE_SIZE

static struct proc_dir_entry *fortune_dir;
static struct proc_dir_entry *fortune_file;
static struct proc_dir_entry *fortune_symlink;
static char *buffer = NULL;

// первый, вызываемый start(), запускает сеанс и принимает позицию
// в качестве аргумента, возвращая итератор, который начнет чтение
// с этой позиции. Позиция, переданная в start(), всегда будет либо
// нулевой, либо самой последней позицией, использованной в предыдущем сеансе.
static void *ct_seq_start(struct seq_file *m, loff_t *pos)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called. %d\n", __func__, (int)pos);

    // loff_t *spos = kzalloc(sizeof(loff_t), GFP_KERNEL);
    if (*pos == 0)
    {
        return buffer;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
    }

    //*pos = 0;
    // *spos = *pos;
    return NULL;
}

// его задача состоит в том, чтобы переместить итератор вперед 
// к следующей позиции в последовательности.
static void *ct_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called. %s ||| %d\n", __func__, (char *)v, (int)pos);

    if (buffer == NULL)
    {
        return NULL;
    }

    // loff_t *spos = v;
    // *pos = ++*spos;    

    return NULL;
}

static void ct_seq_stop(struct seq_file *m, void *v) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
}

static int fortune_show(struct seq_file *sfile, void *v)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called. %s\n", __func__, (char *)v);
    seq_printf(sfile, (char *)v);
    
    return OK;
}

/**
 * Эта структура собирает функции для управления последовательным чтением
 */
const struct seq_operations seq_ops = {
    start: ct_seq_start,
    next: ct_seq_next,
    stop: ct_seq_stop,
    show: fortune_show,
};

/**
 * Эта функция вызывается при открытии файла из /proc
 */
static int fortune_open(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return seq_open(sp_file, &seq_ops);
}

static int fortune_release(struct inode *sp_node, struct file *sp_file) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);
    return OK;
}

static ssize_t fortune_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos) 
{
    printk(KERN_INFO "++ MY_FORTUNE: %s called.\n", __func__);

    if (len > MAX_BUF_SIZE)
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "Buffer overflow");
        return -ENOSPC;
    }

    if (copy_from_user(buffer, buf, len) != 0)
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "copy_from_user function get a error");
        return -EFAULT;
    }

    buffer[len - 1] = '\0';

    return len;
}

/**
 * Эта структура собирает функции для управления файлом из /proc
 */
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

    if ((buffer = vzalloc(MAX_BUF_SIZE)) == NULL)
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "Allocate memory error.");
        return -ENOMEM;
    }

    if ((fortune_dir = proc_mkdir(FORTUNE_DIRNAME, NULL)) == NULL)
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "Error during create directory in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    if ((fortune_file = proc_create(FORTUNE_FILENAME, 0666, fortune_dir, &fops)) == NULL) 
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "Error during create file in proc");
        cleanup_fortune();
        return -ENOMEM;
    }

    if ((fortune_symlink = proc_symlink(FORTUNE_SYMLINK, NULL, FORTUNE_PATH)) == NULL)
    {
        printk(KERN_ERR "FORTUNE_MODULE: %s.\n", "Error during create symlink in proc");
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
