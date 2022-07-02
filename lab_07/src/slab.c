#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");


#define MYFS_MAGIC_NUMBER       0x13131313
#define SLABNAME                "my_cache"


struct myfs_inode
{
    int i_mode; // Права доступа и тип файла
    unsigned long i_ino; // Номер inode
} myfs_inode;


// sco - кол-во вызовов вызовов конструктора
// (сколько раз вызвался конструктор при размещении number объектов)
static int sco = 0;
static int number = 31;
struct kmem_cache *cache = NULL; 
static void **line = NULL;
static int size = sizeof(struct myfs_inode);


void co(void* p)
{ 
    // При размещении объекта вызывается вызывается данная функция.
    *(int*)p = (int)p; 
    sco++; 
} 


static void myfs_put_super(struct super_block* sb)
{
    printk(KERN_INFO ">>> MYFS super block destroyed!\n");
}

// Суперблок - структура, которая описывает ФС (метаданные).
// inode - метаданные о файле (!не содержит путь!) (содержит тип файла, права доступа и тд...)
// dentry (directory entry - запись каталога) - держит дескрипторы и файлы вместе, 
// связывая номер индексных дескрипторов файлов с именами файлов.
static struct super_operations const myfs_super_ops =
{
    // myfs_put_super будет вызываться при размонтировании ФС.
    .put_super = myfs_put_super,
    .statfs = simple_statfs, // реализация системного вызова fstatfs/statfs - для получения статистики ФС, при этом статистика записывается в структуру statfs
                            // simple_statfs - заглушка из стандартной библиотеки 
    .drop_inode = generic_delete_inode, // вызывается, когда исчезает последняя ссылка на inode, удаляется inode
};

static struct inode *myfs_make_inode(struct super_block * sb, int mode)
{
    struct inode *ret = new_inode(sb);

    if (ret)
    {
        inode_init_owner(&init_user_ns, ret, NULL, mode);

        // current_time(ret) - время последнего изменения ret. (т.е. текущее время)
		// mtime - modification time - время последней модификации (изменения) файла
		// atime - access time - время последнего доступа к файлу
		// ctime - change time - время последнего изменения атрибутов файла (данных которые хранятся в inode-области)
		
        ret->i_size = PAGE_SIZE;
        ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
        ret->i_private = &myfs_inode;
    }

    printk(KERN_INFO ">>> New inode was creared\n");

    return ret;
}

static int myfs_fill_sb(struct super_block* sb, void* data, int silent)
{
    struct inode* root = NULL;
    struct dentry *root_dentry;

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    // Магическое число, по которому драйвер файловой системы
	// может проверить, что на диске хранится именно та самая
	// файловая система, а не что-то еще или прочие данные.
    sb->s_magic = MYFS_MAGIC_NUMBER;
    sb->s_op = &myfs_super_ops; // Операции для суперблока.


    // Строим корневой каталог нашей ФС.
	// S_IFDIR - создаем каталог (регулярный).
	// 0755 - стандартные права доступа для папок (rwx r-x r-x). (владелец группа остальные_пользователи)
	
    root = myfs_make_inode(sb, S_IFDIR | 0755);
    if (!root)
    {
        printk (KERN_ERR ">>> MYFS inode allocation failed!\n");
        return -ENOMEM;
    }

    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;

    // суперблок имеет специальное поле, хранящее указатель на dentry корневого каталога.
    root_dentry = d_make_root(root);
    if (!root_dentry)
    {
        printk(KERN_ERR ">>> MYFS root creation failed!\n");
        iput(root);
        return -ENOMEM;
    }
    sb->s_root = root_dentry;

    printk(KERN_INFO ">>> MYFS root creation successed!\n");

    return 0;
}

static struct dentry* myfs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
    // Примонтирует устройство и возвращает структуру, описывающую корневой каталог файловой системы.
	// myfs_fill_sb - функция, которая будет вызвана  из mount_bdev, чтобы проинициализировать суперблок.
    struct dentry* const entry = mount_nodev(type, flags, data, myfs_fill_sb) ;
    
    if (IS_ERR(entry))
        printk(KERN_ERR ">>> MYFS mounting failed !\n") ;
    else
        printk(KERN_DEBUG ">>> MYFS mounted!\n") ;

    return entry;
}


static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = kill_anon_super,
};

static int __init md_init(void)
{
    int ret; 
 
    line = kmalloc(sizeof(void*), GFP_KERNEL); 
    if(!line)
    { 
        printk(KERN_ERR ">>> kmalloc error\n"); 
        return -ENOMEM;
    }
 
    // // Создаем новый кэш.
	// // size - размер элементов кэша.
	// // 0 - смещение первого элемента.
	// // SLAB_HWCACHE_ALIGN — расположение каждого элемента в слабе должно выравниваться по строкам
	// // процессорного кэша, это может существенно поднять производительность, но непродуктивно расходуется память;
	// // co конструктор, который вызывается при размещении каждого элемента.
	cache = kmem_cache_create(SLABNAME, size, 0, 0, co); 
    if(!cache) 
    { 
        printk(KERN_ERR ">>> kmem_cache_create error\n"); 
        kfree(line); 
	    return -ENOMEM;  
    } 

    if (!((*line) = kmem_cache_alloc(cache, GFP_KERNEL)))
    {
        printk(KERN_ERR ">>> kmem_cache_alloc error\n"); 

        kmem_cache_free(cache, *line);
        kmem_cache_destroy(cache);
        kfree(line);

        return -ENOMEM;
    }
        
    ret = register_filesystem(&myfs_type);
    if (ret)
    {
        printk(KERN_ERR ">>> MYFS_MODULE can not register filesystem!\n");
        
        kmem_cache_destroy(cache); 
        kfree(line);

        return ret;
    }

    printk(KERN_INFO ">>> MYFS_MODULE loaded!\n");

    printk(KERN_INFO ">>> allocate %d objects into slab: %s\n", number, SLABNAME); 
	printk(KERN_INFO ">>> object size %d bytes, full size %ld bytes\n", size, (long)size * number); 
	printk(KERN_INFO ">>> constructor called %d times\n", sco); 

    return 0;
}

static void __exit md_exit(void)
{
    kmem_cache_free(cache, *line);
    kmem_cache_destroy(cache); 
    kfree(line);
    
    if (unregister_filesystem(&myfs_type))
        printk(KERN_ERR ">>> MYFS_MODULE can not unregister filesystem!\n");

    printk(KERN_INFO ">>> MYFS_MODULE unloaded!\n");
}


module_init(md_init);
module_exit(md_exit);
