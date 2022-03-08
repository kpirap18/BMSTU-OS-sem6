#include <linux/init_task.h> // init_task
#include <linux/kernel.h> //уровни протоколирования, printk
#include <linux/module.h> // макросы module_init, module_exit, MODULE_LICENSE ...
#include <linux/sched.h> // task_struct

//можно получить с помощью modinfo

// сообщить ядру под какой лицензией распространяется исходный код модуля, 
// что влияет на то, к каким функциям и переменным может получить доступ
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kozlova kpirap18");
MODULE_DESCRIPTION("My first module in lab_03!");

// прототип int func_init(void);
// возврат 0 - успех
// static так как не будет вызываться за пределами модуля и её не нужно экспортировать
// __init - функция используется только на этапе инициализации и освобождает ресурсы после
static int __init md_init(void) 
{
    struct task_struct *task = &init_task;
    printk(KERN_INFO "LAB_03_01: module is unloaded.\n");
    do
    {
        printk(KERN_INFO "LAB_03_01:-----name: %25s\t id: %5d, parent name: %25s\t id: %5d", task->comm,
 		task->pid, task->parent->comm, task->parent->pid);

    } while ((task = next_task(task)) != &init_task);

    // символ current определяет текущий процесс
    printk(KERN_INFO "LAB_03_01:This name: %25s\t id: %5d, parent name: %25s\t id: %5d", current->comm,
 		current->pid, current->parent->comm, current->parent->pid);

    return 0;
}

// прототип void func_exit(void);
static void __exit md_exit(void)  
{ 
    // позволяет отправлять сообщения в системный журнал. 
	// Записывает сообщение в специальный буфер ядра, из буфера их может прочитать демон протоколирования.
	// KERN_INFO - уровень информационного сообщения
    // Одно из отличий функций printk() и printf() заключается в том, 
    // что функция printk() позволяет вам классифицировать сообщения 
    // согласно, назначенным им, приоритетам. Обычно, вы задаете приоритет через макроопределение. 
    printk(KERN_INFO "LAB_03_01: module is unloaded.\n");
}

// регистрация функции инициализации модуля(запрос ресурсов и выделение памяти под структуры)
module_init(md_init);

// регистрация функции которая вызывается при удалении модуля из ядра(освобождение ресурсов)
// после завершения функции модуль выгружается
module_exit(md_exit);