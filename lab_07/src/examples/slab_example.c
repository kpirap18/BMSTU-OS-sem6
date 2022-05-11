#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/moduleparam.h>

#define SLABNAME "my_cache"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Tsiliuric <olej@front.ru>");
MODULE_VERSION("5.2");

static int size = 7; // для наглядности - простые числа
// Передаем параметры модулю (загружаемому модулю ядра).
// (По сути, это полная аналогия с передачей аргументов в программу (argv[])
// Далее значения параметрам могут быть установлены во время загрузки модуля через insmod (см. пример module_param.c).
module_param(size, int, 0); // (имя, тип, права_доступа)
static int number = 31;
module_param(number, int, 0);

static void **line = NULL;
static int sco = 0;

struct kmem_cache *cache = NULL;

static void co(void *p)
{
	// При размещении объекта вызывается вызывается данная функция.
	*(int *)p = (int)p;
	sco++;
}

static int __init init(void)
{
	int i;
	if (size < 0)
	{
		printk(KERN_ERR "invalid argument\n");
		return -EINVAL;
	}
	line = kmalloc(sizeof(void *) * number, GFP_KERNEL); // Get Free Pages
	if (!line)
	{
		printk(KERN_ERR "kmalloc error\n");
		kfree(line);
		return -ENOMEM;
	}
	for (i = 0; i < number; i++)
		line[i] = NULL;

	cache = kmem_cache_create(SLABNAME, size, 0, SLAB_HWCACHE_ALIGN, co);

	if (!cache)
	{
		printk(KERN_ERR "kmem_cache_create error\n");
		kmem_cache_destroy(cache);
		kfree(line);
		return -ENOMEM;
	}

	for (i = 0; i < number; i++)
	{
		if (NULL == (line[i] = kmem_cache_alloc(cache, GFP_KERNEL)))
		{
			printk(KERN_ERR "kmem_cache_alloc error\n");
			for (i = 0; i < number; i++)
				kmem_cache_free(cache, line[i]);
			kmem_cache_destroy(cache);
			kfree(line);
			return -ENOMEM;
		}
	}

	printk(KERN_INFO "allocate %d objects into slab: %s\n", number, SLABNAME);
	printk(KERN_INFO "object size %d bytes, full size %ld bytes\n", size, (long)size * number);
	printk(KERN_INFO "constructor called %d times\n", sco);

	return 0;
}

static void __exit exit(void)
{
	int i;
	for (i = 0; i < number; i++)
		kmem_cache_free(cache, line[i]);
	kmem_cache_destroy(cache);
	kfree(line);
}

module_init(init);
module_exit(exit);
