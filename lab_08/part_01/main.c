#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define IRQ 1  // Прерывание от клавиатуры.
#define POS_MAX 1000
static int devID;

char *my_taskletData = "Key was clicked";
struct proc_dir_entry *proc_file_entry;
/* 
 * count - Счётчик ссылок. если не равен нулю, то тасклет запрещён и не может выполняться
 */
struct tasklet_struct *my_tasklet;

char *simbol[POS_MAX];
int pos_simbol = 0;

void my_tasklet_function(unsigned long data)
{
	// my_tasklet->data++;
    
    int code = inb(0x60);
    char * ascii[84] = 
    {" ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace", 
     "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Ctrl",
     "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "\"", "'", "Shift (left)", "|", 
     "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", "Shift (right)", 
     "*", "Alt", "Space", "CapsLock", 
     "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
     "NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
     " ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"};
    
    if (code < 84) 
    {
        if (pos_simbol < POS_MAX)
        {
            simbol[pos_simbol] = ascii[code];
            pos_simbol++;
        }
        else
        {
            pos_simbol = 0;
        }
        printk(KERN_INFO ">> my_tasklet WHAT IS PRESSED: keyboard %s\n", ascii[code]);
    }

    printk(KERN_INFO "<<< my_tasklet Info about tasklet: State: %ld, Count: %ld, Use_callback: %d, Data: %s", my_tasklet->state, my_tasklet->count,
               my_tasklet->use_callback, (char *)my_tasklet->data);
}



irqreturn_t my_handler(int irq, void *dev)
{
    printk(KERN_INFO "my_tasklet: my_handler...\n");

    if (irq == IRQ)
    {
        // Запланировать тасклет на выполнение.
		// После того, как он запланирован, он добавляется в очередь (на планирование)
		// (состояние меняется на TASKLET_STATE_SCHED).
		// После того, как он запланирован, он выполняется
		// один раз в некоторый момент вермени в будущем.
		// (тасклет всегда выполняется на том процессоре,
		//  который его запланировал на выполнение)
        printk("my_tasklet: my_handler был вызван");
		printk("my_tasklet: Состояние тасклета (ДО ПЛАНИРОВАНИЯ): %lu\n", my_tasklet->state);
        tasklet_schedule(my_tasklet);
        printk("my_tasklet: Состояние тасклета (ПОСЛЕ ПЛАНИРОВАНИЯ): %lu\n", my_tasklet->state);
        return IRQ_HANDLED; // прерывание обработано
    }
    return IRQ_NONE; // прерывание не обработано
}


static int f(struct seq_file *m, void *v)
{
	// Файлы последовательности.
	atomic_t count = my_tasklet->count;

    
	// seq_printf используется для записи данных в оперативную память.
    int i = 0;
    while (i <= pos_simbol)
    {
        seq_printf(m, "%s ", simbol[i]);
        i++;
    }
	
    seq_printf(m, "\n");
	return 0;
}


static int my_proc_open(struct inode *inode, struct file *file)
{
	// логируем f() с помощью single_open ?
	return single_open(file, f, NULL);
}

static const struct proc_ops my_proc_fops =
{
    proc_read: seq_read,
    proc_open: my_proc_open,
    proc_release: single_release,
};


static int __init my_tasklet_init(void)
{
    my_tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);

    if (!my_tasklet)
    {
        printk(KERN_INFO "my_tasklet: kmalloc error!");
        return -1;
    }
    tasklet_init(my_tasklet, my_tasklet_function, (unsigned long)my_taskletData);

    int ret = request_irq(IRQ,              /* номер irq */
                            my_handler,     /* наш обработчик */
                            IRQF_SHARED,    /* линия может быть раздедена, (разрешено совместное использование)*/
                            "my_irq_handler",   /* имя устройства (/proc/interrupts)*/
                            &devID);        /* Последний параметр (идентификатор устройства) irq_handler нужен
										        для того, чтобы можно отключить с помощью free_irq  */
    // NULL - parent_dir (это означает, что наш файл находится в /proc)
	// 0 означает разрешение по умолчанию 0444 ?
	proc_file_entry = proc_create("proc_file_name", 0, NULL, &my_proc_fops);
	if (proc_file_entry == NULL)
		return -ENOMEM;


    if (ret)
    {
        printk(KERN_ERR "my_tasklet: cannot register my_handler\n");
    }
    else
    {
        printk(KERN_INFO "my_tasklet: module loaded\n");
    }
    return ret;
}

static void __exit my_tasklet_exit(void)
{
    synchronize_irq(IRQ);
    tasklet_kill(my_tasklet);
    kfree(my_tasklet);
    free_irq(IRQ, &devID);

    // NULL - parent_dir (это означает, что наш файл находится в /proc)
	remove_proc_entry("proc_file_name", NULL);

    printk(KERN_DEBUG "my_tasklet: module unloaded\n");
}

module_init(my_tasklet_init) 
module_exit(my_tasklet_exit)


