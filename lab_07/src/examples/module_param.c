// Загружаем модуль sudo insmod module_param.ko
// dmesg (значения параметров равны изначальным значениям (т.е. 0))

// Теперь загружаем модуль вот так:
// sudo insmod module_param.ko iparam=3 nparam=4 sparam=str1 cparam=str2 aparam=5,4,3
// dmesg (а теперь мы передали параметры и значения будут такие, которые мы передали)

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Tsiliuric <olej@front.ru>");

static int iparam = 0;
module_param(iparam, int, 0);

static int k = 0; // имена параметра и переменной различаются
module_param_named(nparam, k, int, 0);

static char *sparam = "";
module_param(sparam, charp, 0);

#define FIXLEN 5
static char s[FIXLEN] = ""; // имена параметра и переменной различаются
module_param_string(cparam, s, sizeof(s), 0);

static int aparam[] = {0, 0, 0, 0, 0};
static int arnum = sizeof(aparam) / sizeof(aparam[0]);
module_param_array(aparam, int, &arnum, S_IRUGO | S_IWUSR);

static int __init mod_init(void)
{
	int j;
	char msg[40] = "";
	printk(KERN_INFO "========================================\n");
	printk(KERN_INFO "iparam = %d\n", iparam);
	printk(KERN_INFO "nparam = %d\n", k);
	printk(KERN_INFO "sparam = %s\n", sparam);
	printk(KERN_INFO "cparam = %s {%d}\n", s, strlen(s));
	sprintf(msg, "aparam [ %d ] = ", arnum);
	for (j = 0; j < arnum; j++)
		sprintf(msg + strlen(msg), " %d ", aparam[j]);
	printk(KERN_INFO "%s\n========================================\n", msg);
	return -1;
}

module_init(mod_init);