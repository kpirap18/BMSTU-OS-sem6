// Вызываемый модуль ядра Linux
#include <linux/init.h>	  // ​Макросы __init и ​__exit
#include <linux/module.h> // MODULE_LICENSE, MODULE_AUTHOR

#include "md.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");

// До тех пор, пока число ссылок на любой модуль в системе 
// не станет нулевым, модуль не может быть выгружен;

// API ядра - это множество экспортируемых имён ядра.

char *md1_data = "Привет мир!";

extern char *md1_proc(void)
{
	return md1_data;
}

// Внешние модули могут использовать только те имена, которые явно экспортированы.
// локальное имя непригодное для связывания
static char *md1_local(void)
{
	return md1_data;
}

// не объявлено как экспортируемое имя и не может быть использовано вне модуля
// EXPORT_SYMBOL() помогает вам предоставлять API для других модулей / кода.
// Ваш модуль не загрузится, если он ожидает символ (переменную / функцию) и его нет в ядре.
extern char *md1_noexport(void)
{
	return md1_data;
}

// Любой другой модуль может использовать в своём
// коде любые экспортируемые имена.
EXPORT_SYMBOL(md1_data);
EXPORT_SYMBOL(md1_proc);

static int __init md_init(void)
{
	// "+" - это маркер, отмечающий вывод из собственных модулей.
	// Чтобы дальше можно было легко найти сообщения с помощью grep.
	printk(KERN_INFO "++ module md1 start!\n");
	printk(KERN_INFO "++ module md1 use local from md1: %s\n", md1_local());
    printk(KERN_INFO "++ module md1 use noexport from md1: %s\n", md1_noexport());
	return 0;
}

static void __exit md_exit(void)
{
	printk(KERN_INFO "+ module md1 unloaded!\n");
}

module_init(md_init);
module_exit(md_exit);