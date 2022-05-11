#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/vmalloc.h>
#include <linux/proc_fs.h> 
#include <asm/uaccess.h>
// #include <unistd.h>
#include <linux/stringhash.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("kpirap18");

#define MYFS_MAGIC 0xDADBABE
#define TMPSIZE 128

static atomic_t counter, subcounter;

static void myfs_put_super(struct super_block *sb)
{
    printk(KERN_DEBUG "+: myfs super block destroyed\n");
}

static struct super_operations lfs_s_ops = {
    .put_super = myfs_put_super,
    .statfs		=simple_statfs,
    .drop_inode	=generic_delete_inode,
};

// inode
static struct inode *lfs_make_inode(struct super_block *sb, int mode) {
    struct inode *ret = new_inode(sb);

    inode_init_owner(&init_user_ns, ret, NULL, mode);
    if (ret) {
        ret->i_mode = mode;
        // ret->i_uid = ret->i_gid = 0;
        ret->i_size = PAGE_SIZE;
        ret->i_blocks = 0;
        ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
    }
    return ret;
}

// операции над файлами
static int lfs_open(struct inode *inode, struct file *filp) {
    // filp->private_data = inode->u.generic_ip;
    return 0;
}

// возможны гонки
static ssize_t lfs_read_file(struct file *filp, char *buf,
					size_t count, loff_t *offset) {

    atomic_t *counter = (atomic_t *) filp->private_data;
    int v, len;
    char tmp[TMPSIZE];

    v = atomic_read(counter);
    if (*offset > 0)
        v -= 1;  
    else
        atomic_inc(counter);
    len = snprintf(tmp, TMPSIZE, "%d\n", v);
    if (*offset > len)
        return 0;
    if (count > len - *offset)
    count = len - *offset;

    if (copy_to_user(buf, tmp + *offset, count))
        return -EFAULT;
    *offset += count;
    return count;
}

static ssize_t lfs_write_file(struct file *filp, const char *buf,
					size_t count, loff_t *offset) {

    atomic_t *counter = (atomic_t *) filp->private_data;
    char tmp[TMPSIZE];

    if (*offset != 0)
    return -EINVAL;

    if (count >= TMPSIZE)
        return -EINVAL;
    memset(tmp, 0, TMPSIZE);
    if (copy_from_user(tmp, buf, count))
        return -EFAULT;

    atomic_set(counter, simple_strtol(tmp, NULL, 10));
    return count;
}

// static const struct proc_ops lfs_file_ops =
// {
//     proc_read: lfs_read_file,
//     proc_write: lfs_write_file,
//     proc_open: lfs_open,
// };


static struct file_operations lfs_file_ops = {
	    .open		=lfs_open,
	    .read		=lfs_read_file,
	    .write	=lfs_write_file,
	};

// файлы
static struct dentry *lfs_create_file(struct super_block *sb,
            struct dentry *dir, const char *name,
            atomic_t *counter) {

    struct dentry	*dentry;
    struct inode	*inode;
    struct qstr		qname;

    /* Инициализируем qstr, считаем хэш */
    qname.name = name;
    qname.len = strlen (name);
    void *d;
    qname.hash = full_name_hash(d, name, qname.len);

    /* Создаем dentry для файла */
    dentry = d_alloc(dir, &qname);
    if (! dentry)
        goto out;

    /* Создаем inode для файла */
    inode = lfs_make_inode(sb, S_IFREG | 0644);
    if (! inode)
        goto out_dput;
    inode->i_fop = &lfs_file_ops;
    // inode->u.generic_ip = counter;

    d_add(dentry, inode);
    return dentry;

out_dput:
    dput(dentry);
out:
    return 0;
}


static struct dentry *lfs_create_dir (struct super_block *sb,
    struct dentry *parent, const char *name)
{
    struct dentry	*dentry;
    struct inode	*inode;
    struct qstr		qname;

    qname.name = name;
    qname.len = strlen (name);
    void *d;
    qname.hash = full_name_hash(d, name, qname.len);

    dentry = d_alloc(parent, &qname);
    if (! dentry)
        goto out;

    inode = lfs_make_inode(sb, S_IFDIR | 0644);
    if (! inode)
        goto out_dput;
    inode->i_op = &simple_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;

    d_add(dentry, inode);
    return dentry;

out_dput:
    dput(dentry);
out:
    return 0;
}

static void lfs_create_files(struct super_block *sb, struct dentry *root) {
    struct dentry *subdir;

    /* Создаем файл "counter" в корневом каталоге */
    atomic_set(&counter, 0);
    lfs_create_file(sb, root, "counter", &counter);

    /* Создаем каталог "subdir" */
    atomic_set(&subcounter, 0);
    subdir = lfs_create_dir(sb, root, "subdir");

    /* Создаем файл "subcounter" в "subdir" */
    if (subdir)
        lfs_create_file(sb, subdir, "subcounter", &subcounter);
}






static int lfs_fill_super (struct super_block *sb, void *data, int silent) {
    struct inode	*root;
    struct dentry	*root_dentry;

    /* Устанавливаем поля суперблока */
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC;
    sb->s_op = &lfs_s_ops;

    /* Создание inode для корневого каталога */
    root = lfs_make_inode (sb, S_IFDIR | 0755);
    if (! root)
        goto out;
    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;

    /* Создание dentry для корневого каталога */
    root_dentry = d_make_root(root);
    if (! root_dentry)
        goto out_iput;
    sb->s_root = root_dentry;

    /* Создание логической структуры файлов и папок */
    lfs_create_files (sb, root_dentry);
    return 0;

out_iput:
    /* Если выделение dentry провалилось, уничтожаем 
    * inode и выходим */
    iput(root);
out:
    return -ENOMEM;
}


// static struct super_block *lfs_get_super(struct file_system_type *fst, 
//                 int flags, const char *devname, void *data) {
//     return get_sb_single(fst,flags,data,lfs_fill_super);
// }

static struct dentry *myfs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
    struct dentry *const entry = mount_nodev(type, flags, data, lfs_fill_super);
    if (IS_ERR(entry))
    {
        printk(KERN_ERR "+: myfs mounting failed\n");
    }
    else
    {
        printk(KERN_DEBUG "+: myfs mounted\n");
    }

    return entry;
}

static struct file_system_type lfs_type = {
    .owner          = THIS_MODULE,
    .name           = "lwnfs",
    // .get_sb         = lfs_get_super,
    .mount = myfs_mount,
    .kill_sb        = kill_litter_super, // include/linux/fs.h
};

static int __init lfs_init(void) {
    return register_filesystem(&lfs_type); // include/linux/fs.h
}


static void __exit lfs_exit(void) {
    unregister_filesystem(&lfs_type);
}

module_init(lfs_init);
module_exit(lfs_exit);




