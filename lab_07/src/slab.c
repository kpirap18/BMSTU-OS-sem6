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
    int i_mode;
    unsigned long i_ino;
} myfs_inode;


static int sco = 0;
static int number = 31;
struct kmem_cache *cache = NULL; 
static void **line = NULL;
static int size = sizeof(struct myfs_inode);


void co(void* p)
{ 
    *(int*)p = (int)p; 
    sco++; 
} 


static void myfs_put_super(struct super_block* sb)
{
    printk(KERN_INFO ">>> MYFS super block destroyed!\n");
}

static struct super_operations const myfs_super_ops =
{
    .put_super = myfs_put_super,
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

static struct inode *myfs_make_inode(struct super_block * sb, int mode)
{
    struct inode *ret = new_inode(sb);

    if (ret)
    {
        inode_init_owner(&init_user_ns, ret, NULL, mode);

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

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC_NUMBER;
    sb->s_op = &myfs_super_ops;

    root = myfs_make_inode(sb, S_IFDIR | 0755);
    if (!root)
    {
        printk (KERN_ERR ">>> MYFS inode allocation failed!\n");
        return -ENOMEM;
    }

    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;
    sb->s_root = d_make_root(root);

    if (!sb->s_root)
    {
        printk(KERN_ERR ">>> MYFS root creation failed!\n");
        iput(root);
        return -ENOMEM;
    }

    printk(KERN_INFO ">>> MYFS root creation successed!\n");

    return 0;
}

static struct dentry* myfs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
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