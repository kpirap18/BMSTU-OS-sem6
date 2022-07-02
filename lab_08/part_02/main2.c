#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

#define IRQ 1
static int devID;

struct workqueue_struct *workQueue;
char *simbol;


void queue_functionF(struct work_struct *work)
{
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
        simbol = ascii[code];
        printk(KERN_INFO ">> my_Queue WHAT IS PRESSED: keyboard %s\n", ascii[code]);
    }
    printk(KERN_INFO "<<<my_Queue: Key was clicked (1 worker) and we sleep\n");
    msleep(10);
    printk(KERN_INFO "<<<my_Queue: Key was clicked (1 worker) return\n");
}

void queue_functionS(struct work_struct *work)
{
    // For kernel 5.4
	atomic64_t data64 = work->data;
	long long data = data64.counter;
	// For kernel 5.4

	printk(KERN_INFO "<<<my_Queue: queue_functionS data = %lld\n", data);
    printk(KERN_INFO "<<<my_Queue: Key was clicked (2 worker)\n");
}

struct work_struct fWork;
struct work_struct sWork;

irqreturn_t handler(int irq, void *dev)
{
    printk(KERN_INFO "<<<my_Queue: move work to queue...\n");
    if (irq == IRQ)
    {
        // Помещаем структуру в очередь работ.
		// queue_work назначает работу текущему процессору.
        queue_work(workQueue, &fWork);
        queue_work(workQueue, &sWork);
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}


static int __init work_queue_init(void)
{
    int ret = request_irq(IRQ,      /* номер irq */
                    handler,        /* наш обработчик */
                    IRQF_SHARED,    /* линия может быть раздедена, IRQ
										(разрешено совместное использование)*/
                    "my_irq2_handler",      /* имя устройства (можно потом посмотреть в /proc/interrupts)*/
                    &devID);        /* Последний параметр (идентификатор устройства) irq_handler нужен
										для того, чтобы можно отключить с помощью free_irq  */
	
    if (ret)
    {
        printk(KERN_ERR "<<<my_Queue: handler wasn't registered\n");
        return ret;
    }

    if (!(workQueue = create_workqueue("my_queue"))) //создание очереди работ
    {
        free_irq(IRQ, &devID);
        printk(KERN_INFO "<<<my_Queue: workqueue wasn't created");
        return -ENOMEM;
    }

    INIT_WORK(&fWork, queue_functionF);
    INIT_WORK(&sWork, queue_functionS);

    printk(KERN_INFO "<<<my_Queue: module loaded\n");
    return 0;
}

static void __exit work_queue_exit(void)
{
    // Принудительно завершаем все работы в очереди.
	// Вызывающий блок блокируется до тех пор, пока операция не будет завершена.
    flush_workqueue(workQueue);
    destroy_workqueue(workQueue);
    free_irq(IRQ, &devID);
    // remove_proc_entry("workqueue", NULL);
    printk(KERN_INFO "<<<my_Queue: module unloaded\n");
}

module_init(work_queue_init) 
module_exit(work_queue_exit)