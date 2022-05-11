#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>
// #include <unistd.h>

#define CACHE_SIZE 128
MODULE_LICENSE("GPL");
static struct kmem_cache *inode_cache = NULL;
static struct myfs_inode **inode_pointers = NULL;
static int c_size = CACHE_SIZE;
static int cached_count = 0;


#define MYFS_MAGIC_NUMBER       0x13131313
#define SLABNAME                "my_cache"
#define MYFS_FILENAME_LEN strlen(SLABNAME)

#define MYRWFS_BLOCK_SIZE_BYTES 128

#define ROOT_INODE_NUM (1)
#define MYRWFS_TO_VFS_INODE_NUM(i) (ROOT_INODE_NUM + 1 + (i))
#define VFS_TO_MYRWFS_INODE_NUM(i) ((i) - (ROOT_INODE_NUM + 1))

#define SIMULA_FS_ENTRY_SIZE 64 /* in bytes */
#define MYFS_DATA_BLOCK_CNT ((SIMULA_FS_ENTRY_SIZE - (16 + 3 * 4)) / 4)
// источник: https://sysplay.in/blog/linux-device-drivers/2014/07/file-systems-the-semester-project/
#define MYRWFS_ENTRY_FRACTION 0.10 /* 10% блоков резервируем под таблицу дескрипторов - эмпирически */

#define MYRWFS_ENTRY_TABLE_BLOCK_START 1


struct inode *myfs_root_inode;


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

// от Даши
typedef struct myfs_super_block
{
    loff_t magic_number; /* Magic number - идентифицировать файловую систему */
    loff_t block_size_bytes; /* размер блока в байтах */
    loff_t bdev_size_blocks; /* размер устройства в блоках */
    loff_t entry_size_bytes; /* размер дескриптора файла в байтах */
    loff_t entry_table_size_blocks; /* размер таблицы дескрипторов в блоках */
    loff_t entry_table_block_start; /* смещение к началу таблицы дескрипторов в блоках - она расположена после суперблока на устройстве */
    loff_t entry_count; /* количество дескрипторов в таблице дескрипторов */
    loff_t data_block_start; /* смещение к началу данных файловой системы на блочном устройстве */
    loff_t reserved[MYRWFS_BLOCK_SIZE_BYTES / 4 - 8];
} myfs_super_block_t;

// от Даши
typedef struct myfs_file_entry
{
    // char name[MYFS_FILENAME_LEN + 1];
    loff_t size; /* размер дескриптора файла в байтах */
    loff_t perms; /* разрешения */
    loff_t blocks[MYFS_DATA_BLOCK_CNT];
} myfs_file_entry_t;

// от Даши
typedef struct myfs_info
{
    struct super_block *vfs_sb; /* обратный указатель на структуру суперблок пространства ядра */
    myfs_super_block_t myfs_sb; /* указатель на структуру суперблок, расположенную на устройстве */
    loff_t *used_blocks; /* указатель на количество блоков, помеченных как занятые */
} myfs_info_t;



static int myfs_fill_super(struct super_block *sb, void *data, int silent);
static int myfs_inode_create(struct user_namespace *k, struct inode *parent_inode, struct dentry *dentry, umode_t mode, bool excl);
static struct dentry *myfs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned int flags);
int myfs_create(myfs_info_t *info, char *fn, int perms, myfs_file_entry_t *fe);
static int read_entry_from_myfs(myfs_info_t *info, int ino, myfs_file_entry_t *fe);
static int write_entry_to_myfs(myfs_info_t *info, int ino, myfs_file_entry_t *fe);
int myfs_lookup(myfs_info_t *info, char *fn, myfs_file_entry_t *fe);



static struct file_operations myfs = {
	    // .open		=lfs_open,
	    .read		= (size_t)read_entry_from_myfs,
	    .write	= (size_t)write_entry_to_myfs,
	};




// было
void co(void* p)
{ 
    *(int*)p = (int)p; 
    sco++; 
} 

// от Даши
static void myfs_put_super(struct super_block* sb)
{
    myfs_info_t *info = (myfs_info_t *)(sb->s_fs_info);

    printk(KERN_INFO "** MYFS: myfs_put_super\n");
    // if (info) {
    //     kill_myfs_info(info);
    //     kfree(info);
    //     sb->s_fs_info = NULL;
    // }
}

// тако же
static struct super_operations const myfs_super_ops =
{
    .put_super = myfs_put_super,
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};


// Дашина myfs_mount
static struct dentry* myfs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
    // printk(KERN_INFO "** MYFS: myfs_mount: devname = %s\n", devname);

    struct dentry* const entry = mount_nodev(type, flags, data, &myfs_fill_super) ;
    
    if (IS_ERR(entry))
        printk(KERN_ERR ">>> MYFS mounting failed !\n") ;
    else
        printk(KERN_DEBUG ">>> MYFS mounted!\n") ;

    return entry;
}




static int myfs_inode_create(struct user_namespace *k, struct inode *parent_inode, struct dentry *dentry, umode_t mode, bool excl) {
    char fn[dentry->d_name.len + 1];
    int perms = 0;
    myfs_info_t *info = (myfs_info_t *)(parent_inode->i_sb->s_fs_info);
    int ino;
    struct inode *file_inode;
    myfs_file_entry_t fe;

    printk(KERN_INFO "** MYRWFS: myfs_inode_create\n");

    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;
    if (mode & (S_IRUSR | S_IRGRP | S_IROTH))
        mode |= (S_IRUSR | S_IRGRP | S_IROTH);
    if (mode & (S_IWUSR | S_IWGRP | S_IWOTH))
        mode |= (S_IWUSR | S_IWGRP | S_IWOTH);
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        mode |= (S_IXUSR | S_IXGRP | S_IXOTH);
    perms |= (mode & S_IRUSR) ? 4 : 0;
    perms |= (mode & S_IWUSR) ? 2 : 0;
    perms |= (mode & S_IXUSR) ? 1 : 0;
    if ((ino = myfs_create(info, fn, perms, &fe)) == -1)
        return -1;

    file_inode = new_inode(parent_inode->i_sb);
    // if (!file_inode)
    // {
    //     myfs_remove(info, fn);
    //     return -ENOMEM;
    // }
    printk(KERN_INFO "** MYRWFS: Created new VFS inode for #%d, let's fill in\n", ino);
    file_inode->i_ino = ino;
    file_inode->i_size = fe.size;
    file_inode->i_mode = S_IFREG | mode;
    file_inode->i_fop = &myfs;
    // if (insert_inode_locked(file_inode) < 0)
    // {
    //     make_bad_inode(file_inode);
    //     iput(file_inode);
    //     myfs_remove(info, fn);
    //     return -1;
    // }
    d_instantiate(dentry, file_inode);
    unlock_new_inode(file_inode);

    return 0;
}

static struct dentry *myfs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned int flags) {
    myfs_info_t *info = (myfs_info_t *)(parent_inode->i_sb->s_fs_info);
    char fn[dentry->d_name.len + 1];
    int ino;
    myfs_file_entry_t fe;
    struct inode *file_inode = NULL;

    printk(KERN_INFO "** MYRWFS: myfs_inode_lookup\n");

    if (parent_inode->i_ino != myfs_root_inode->i_ino)
        return ERR_PTR(-ENOENT);

    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;
    if ((ino = myfs_lookup(info, fn, &fe)) == -1)
      return d_splice_alias(file_inode, dentry);

    file_inode = iget_locked(parent_inode->i_sb, ino);
    if (!file_inode)
        return ERR_PTR(-EACCES);

    if (file_inode->i_state & I_NEW) {
        file_inode->i_size = fe.size;
        file_inode->i_mode = S_IFREG;
        file_inode->i_mode |= ((fe.perms & 4) ? S_IRUSR | S_IRGRP | S_IROTH : 0);
        file_inode->i_mode |= ((fe.perms & 2) ? S_IWUSR | S_IWGRP | S_IWOTH : 0);
        file_inode->i_mode |= ((fe.perms & 1) ? S_IXUSR | S_IXGRP | S_IXOTH : 0);
        file_inode->i_fop = &myfs;
        unlock_new_inode(file_inode);
    }

    d_add(dentry, file_inode);
    return NULL;
}



static struct inode_operations myfs_iops = {
    create: myfs_inode_create,
    lookup: myfs_inode_lookup
};




// от Даши
static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .fs_flags = FS_REQUIRES_DEV,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = kill_block_super, // было kill_anon_super
};
// от Даши
static int myfs_fill_super(struct super_block *sb, void *data, int silent) {
   
    printk(KERN_INFO "** MYFS: myfs_fill_super\n");
   
    myfs_info_t *info;
    if (!(info = (myfs_info_t *)(kzalloc(sizeof(myfs_info_t), GFP_KERNEL))))
        return -ENOMEM;

    info->vfs_sb = sb;
    // if (fill_myfs_info(info) < 0) {
    //     kfree(info);
    //     return -1;
    // }
   
    sb->s_magic = info->myfs_sb.magic_number;
    sb->s_blocksize = info->myfs_sb.block_size_bytes;
    // sb->s_blocksize_bits = log_base_2(info->myfs_sb.block_size_bytes);
    sb->s_type = &myfs_type;
    sb->s_op = &myfs_super_ops;

    myfs_root_inode = iget_locked(sb, ROOT_INODE_NUM);
    // if (!myfs_root_inode) {
    //     kill_myfs_info(info);
    //     kfree(info);
    //     return -1;
    // }

    if (myfs_root_inode->i_state & I_NEW) {
        myfs_root_inode->i_op = &myfs_iops;
        myfs_root_inode->i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        myfs_root_inode->i_fop = &myfs;
        unlock_new_inode(myfs_root_inode);
    }

    sb->s_root = d_make_root(myfs_root_inode);
    // if (!sb->s_root) {
    //     iget_failed(myfs_root_inode);
    //     kill_mywfs_info(info);
    //     kfree(info);
    //     return -ENOMEM;
    // }

    return 0;
}

// static struct inode *myfs_make_inode(struct super_block * sb, int mode)
// {
//     struct inode *ret = new_inode(sb);

//     if (ret)
//     {
//         inode_init_owner(&init_user_ns, ret, NULL, mode);

//         ret->i_size = PAGE_SIZE;
//         ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
//         ret->i_private = &myfs_inode;
//     }

//     printk(KERN_INFO ">>> New inode was creared\n");

//     return ret;
// }

// static int myfs_fill_sb(struct super_block* sb, void* data, int silent)
// {
//     struct inode* root = NULL;

//     sb->s_blocksize = PAGE_SIZE;
//     sb->s_blocksize_bits = PAGE_SHIFT;
//     sb->s_magic = MYFS_MAGIC_NUMBER;
//     sb->s_op = &myfs_super_ops;

//     root = myfs_make_inode(sb, S_IFDIR | 0755);
//     if (!root)
//     {
//         printk (KERN_ERR ">>> MYFS inode allocation failed!\n");
//         return -ENOMEM;
//     }

//     root->i_op = &simple_dir_inode_operations;
//     root->i_fop = &simple_dir_operations;
//     sb->s_root = d_make_root(root);

//     if (!sb->s_root)
//     {
//         printk(KERN_ERR ">>> MYFS root creation failed!\n");
//         iput(root);
//         return -ENOMEM;
//     }

//     printk(KERN_INFO ">>> MYFS root creation successed!\n");

//     return 0;
// }


// от Даши
// void write_super_block(int fd, myfs_super_block_t *myfs_sb) {
//     write(fd, myfs_sb, sizeof(myfs_super_block_t));
// }

// от Даши
void write_file_entries_table(int fd, myfs_super_block_t *myfs_sb) {
    loff_t block[MYRWFS_BLOCK_SIZE_BYTES];

    for (int i = 0; i < myfs_sb->block_size_bytes / myfs_sb->entry_size_bytes; i++) {
        memcpy(block + i * myfs_sb->entry_size_bytes, &fe, sizeof(fe));
    }

    for (int i = 0; i < myfs_sb->entry_table_size_blocks; i++) {
        write(fd, block, sizeof(block));
    }
}


static int read_sb_from_myfs(myfs_info_t *info, myfs_super_block_t *myfs_sb) {
    struct buffer_head *bh;
    if (!(bh = sb_bread(info->vfs_sb, 0))) {
        return -1;
    }
    memcpy(myfs_sb, bh->b_data, MYRWFS_BLOCK_SIZE_BYTES);
    brelse(bh);
    return 0;
}

static int read_entry_from_myfs(myfs_info_t *info, int ino, myfs_file_entry_t *fe) {
    loff_t offset = ino * sizeof(myfs_file_entry_t);
    loff_t len = sizeof(myfs_file_entry_t);
    loff_t block = info->myfs_sb.entry_table_block_start;
    loff_t block_size_bytes = info->myfs_sb.block_size_bytes;
    loff_t bd_block_size = info->vfs_sb->s_bdev->bd_block_size;
    loff_t abs;
    struct buffer_head *bh;

    abs = block * block_size_bytes + offset;
    block = abs / bd_block_size;
    offset = abs % bd_block_size;
    if (offset + len > bd_block_size) {
        return -1;
    }

    if (!(bh = sb_bread(info->vfs_sb, block))) {
        return -1;
    }

    memcpy((void *) fe, bh->b_data + offset, len);
    brelse(bh);

    return 0;
}

static int write_entry_to_myfs(myfs_info_t *info, int ino, myfs_file_entry_t *fe) {
    loff_t offset = ino * sizeof(myfs_file_entry_t);
    loff_t len = sizeof(myfs_file_entry_t);
    loff_t block = info->myfs_sb.entry_table_block_start;
    loff_t block_size_bytes = info->myfs_sb.block_size_bytes;
    loff_t bd_block_size = info->vfs_sb->s_bdev->bd_block_size;
    struct buffer_head *bh;

    loff_t abs = block * block_size_bytes + offset;
    block = abs / bd_block_size;
    offset = abs % bd_block_size;
    if (offset + len > bd_block_size) {
        return -1;
    }

    if (!(bh = sb_bread(info->vfs_sb, block))) {
        return -1;
    }

    memcpy(bh->b_data + offset, (void *) fe, len);
    mark_buffer_dirty(bh);
    brelse(bh);
    return 0;
}

int myfs_create(myfs_info_t *info, char *fn, int perms, myfs_file_entry_t *fe) {
    int ino, free_ino, i;

    free_ino = -1;
    for (ino = 0; ino < info->myfs_sb.entry_count; ino++) {
       
        if (read_entry_from_myfs(info, ino, fe) < 0)
            return -1;
       
        if (!fe->name[0]) {
            free_ino = ino;
            break;
        }
    }
   
    if (free_ino == -1) {
        printk(KERN_ERR "No entries left\n");
        return -1;
    }

    strncpy(fe->name, fn, MYFS_FILENAME_LEN);
    fe->name[MYFS_FILENAME_LEN] = 0;
    fe->size = 0;
    fe->perms = perms;
    for (i = 0; i < MYFS_DATA_BLOCK_CNT; i++) {
        fe->blocks[i] = 0;
    }

    if (write_entry_to_myfs(info, free_ino, fe) < 0)
        return -1;

    return MYRWFS_TO_VFS_INODE_NUM(free_ino);
}

int myfs_lookup(myfs_info_t *info, char *fn, myfs_file_entry_t *fe) {
    for (int ino = 0; ino < info->myfs_sb.entry_count; ino++) {
       
        if (read_entry_from_myfs(info, ino, fe) < 0)
            return -1;
       
        if (!fe->name[0])
            continue;
       
        if (strcmp(fe->name, fn) == 0)
            return MYRWFS_TO_VFS_INODE_NUM(ino);
    }

    return -1;
}


int fill_myfs_info(myfs_info_t *info) {

    if (read_sb_from_myfs(info, &info->myfs_sb) < 0) {
        return -1;
    }

    if (info->myfs_sb.magic_number != MYFS_MAGIC_NUMBER) {
        printk(KERN_ERR "Invalid MYRWFS detected. Giving up.\n");
        return -1;
    }

    loff_t *used_blocks = (loff_t *)(kmalloc(info->myfs_sb.bdev_size_blocks));
    if (!used_blocks) {
        return -ENOMEM;
    }

    int i;
    for (i = 0; i < info->myfs_sb.data_block_start; i++) {
        used_blocks[i] = 1;
    }
    for (; i < info->myfs_sb.bdev_size_blocks; i++) {
        used_blocks[i] = 0;
    }

    myfs_file_entry_t fe;
    for (i = 0; i < info->myfs_sb.entry_count; i++) {
        // if (read_entry_from_myfs(info, i, &fe) < 0) {
        //     vfree(used_blocks);
        //     return -1;
        // }
       
        // if (!fe.name[0])
        //     continue;
       
        for (int j = 0; j < MYFS_DATA_BLOCK_CNT; j++) {
            if (fe.blocks[j] == 0) break;
            used_blocks[fe.blocks[j]] = 1;
        }
    }

    info->used_blocks = used_blocks;
    info->vfs_sb->s_fs_info = info;
    return 0;
}

void kill_mywfs_info(myfs_info_t *info) {
    if (info->used_blocks)
        kfree(info->used_blocks);
}



int myfs_get_data_block(myfs_info_t *info) {
    int i;

    for (i = info->myfs_sb.data_block_start; i < info->myfs_sb.bdev_size_blocks; i++) {
        if (info->used_blocks[i] == 0) {
            info->used_blocks[i] = 1;
            return i;
        }
    }
    return -1;
}

void myfs_put_data_block(myfs_info_t *info, int i) {
    info->used_blocks[i] = 0;
}




























// ДЛЯ ЗАГРУЗКИ 
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