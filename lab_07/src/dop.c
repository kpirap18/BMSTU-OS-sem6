#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>

#define CACHE_SIZE 128

static struct kmem_cache *inode_cache = NULL;
static struct myfs_inode **inode_pointers = NULL;
static int c_size = CACHE_SIZE;
static int cached_count = 0;

typedef struct myrwfs_super_block
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
} myrwfs_super_block_t;

typedef struct myrwfs_file_entry
{
    char name[MYRWFS_FILENAME_LEN + 1];
    loff_t size; /* размер дескриптора файла в байтах */
    loff_t perms; /* разрешения */
    loff_t blocks[MYRWFS_DATA_BLOCK_CNT];
} myrwfs_file_entry_t;

typedef struct myrwfs_info
{
    struct super_block *vfs_sb; /* обратный указатель на структуру суперблок пространства ядра */
    myrwfs_super_block_t myrwfs_sb; /* указатель на структуру суперблок, расположенную на устройстве */
    loff_t *used_blocks; /* указатель на количество блоков, помеченных как занятые */
} myrwfs_info_t;

#define ROOT_INODE_NUM (1)
#define MYRWFS_TO_VFS_INODE_NUM(i) (ROOT_INODE_NUM + 1 + (i))
#define VFS_TO_MYRWFS_INODE_NUM(i) ((i) - (ROOT_INODE_NUM + 1))


// источник: https://sysplay.in/blog/linux-device-drivers/2014/07/file-systems-the-semester-project/
#define MYRWFS_ENTRY_FRACTION 0.10 /* 10% блоков резервируем под таблицу дескрипторов - эмпирически */

#define MYRWFS_ENTRY_TABLE_BLOCK_START 1

// static void myfs_put_super(struct super_block *sb)
// {
//     printk(KERN_DEBUG "+: myfs super block destroyed\n");
// }

// static struct super_operations const myrwfs_sops = {
//     .put_super = myfs_put_super,
//     .statfs = simple_statfs,
//     .drop_inode = generic_delete_inode,
// };


static int myrwfs_fill_super(struct super_block *sb, void *data, int silent) {
   
    printk(KERN_INFO "** MYRWFS: myrwfs_fill_super\n");
   
    myrwfs_info_t *info;
    if (!(info = (myrwfs_info_t *)(kzalloc(sizeof(myrwfs_info_t), GFP_KERNEL))))
        return -ENOMEM;

    info->vfs_sb = sb;
    if (fill_myrwfs_info(info) < 0) {
        kfree(info);
        return -1;
    }
   
    sb->s_magic = info->myrwfs_sb.magic_number;
    sb->s_blocksize = info->myrwfs_sb.block_size_bytes;
    sb->s_blocksize_bits = log_base_2(info->myrwfs_sb.block_size_bytes);
    sb->s_type = &myrwfs;
    sb->s_op = &myrwfs_sops;

    myrwfs_root_inode = iget_locked(sb, ROOT_INODE_NUM);
    if (!myrwfs_root_inode) {
        kill_mywfs_info(info);
        kfree(info);
        return -1;
    }

    if (myrwfs_root_inode->i_state & I_NEW) {
        myrwfs_root_inode->i_op = &myrwfs_iops;
        myrwfs_root_inode->i_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        myrwfs_root_inode->i_fop = &myrwfs_dops;
        unlock_new_inode(myrwfs_root_inode);
    }

    sb->s_root = d_make_root(myrwfs_root_inode);
    if (!sb->s_root) {
        iget_failed(myrwfs_root_inode);
        kill_mywfs_info(info);
        kfree(info);
        return -ENOMEM;
    }

    return 0;
}

static void myrwfs_put_super(struct super_block *sb) {
    myrwfs_info_t *info = (myrwfs_info_t *)(sb->s_fs_info);

    printk(KERN_INFO "** MYRWFS: myrwfs_put_super\n");
    if (info) {
        kill_mywfs_info(info);
        kfree(info);
        sb->s_fs_info = NULL;
    }
}

static struct dentry *myrwfs_mount(struct file_system_type *fs, int flags, const char *devname, void *data) {
    printk(KERN_INFO "** MYRWFS: myrwfs_mount: devname = %s\n", devname);
    return mount_bdev(fs, flags, devname, data, &myrwfs_fill_super);
}

static struct file_system_type myrwfs = {
    name: "myrwfs",
    fs_flags: FS_REQUIRES_DEV,
    mount:  myrwfs_mount,
    kill_sb: kill_block_super,
    owner: THIS_MODULE
};


void write_super_block(int fd, myrwfs_super_block_t *myrwfs_sb) {
    write(fd, myrwfs_sb, sizeof(myrwfs_super_block_t));
}

void write_file_entries_table(int fd, myrwfs_super_block_t *myrwfs_sb) {
    loff_t block[MYRWFS_BLOCK_SIZE_BYTES];

    for (int i = 0; i < myrwfs_sb->block_size_bytes / myrwfs_sb->entry_size_bytes; i++) {
        memcpy(block + i * myrwfs_sb->entry_size_bytes, &fe, sizeof(fe));
    }

    for (int i = 0; i < myrwfs_sb->entry_table_size_blocks; i++) {
        write(fd, block, sizeof(block));
    }
}


static int read_sb_from_myrwfs(myrwfs_info_t *info, myrwfs_super_block_t *myrwfs_sb) {
    struct buffer_head *bh;
    if (!(bh = sb_bread(info->vfs_sb, 0))) {
        return -1;
    }
    memcpy(myrwfs_sb, bh->b_data, MYRWFS_BLOCK_SIZE_BYTES);
    brelse(bh);
    return 0;
}

static int read_entry_from_myrwfs(myrwfs_info_t *info, int ino, myrwfs_file_entry_t *fe) {
    loff_t offset = ino * sizeof(myrwfs_file_entry_t);
    loff_t len = sizeof(myrwfs_file_entry_t);
    loff_t block = info->myrwfs_sb.entry_table_block_start;
    loff_t block_size_bytes = info->myrwfs_sb.block_size_bytes;
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

static int write_entry_to_myrwfs(myrwfs_info_t *info, int ino, myrwfs_file_entry_t *fe) {
    loff_t offset = ino * sizeof(myrwfs_file_entry_t);
    loff_t len = sizeof(myrwfs_file_entry_t);
    loff_t block = info->myrwfs_sb.entry_table_block_start;
    loff_t block_size_bytes = info->myrwfs_sb.block_size_bytes;
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


   --------



int myrwfs_create(myrwfs_info_t *info, char *fn, int perms, myrwfs_file_entry_t *fe) {
    int ino, free_ino, i;

    free_ino = -1;
    for (ino = 0; ino < info->myrwfs_sb.entry_count; ino++) {
       
        if (read_entry_from_myrwfs(info, ino, fe) < 0)
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

    strncpy(fe->name, fn, MYRWFS_FILENAME_LEN);
    fe->name[MYRWFS_FILENAME_LEN] = 0;
    fe->size = 0;
    fe->perms = perms;
    for (i = 0; i < MYRWFS_DATA_BLOCK_CNT; i++) {
        fe->blocks[i] = 0;
    }

    if (write_entry_to_myrwfs(info, free_ino, fe) < 0)
        return -1;

    return MYRWFS_TO_VFS_INODE_NUM(free_ino);
}

int myrwfs_lookup(myrwfs_info_t *info, char *fn, myrwfs_file_entry_t *fe) {
    for (int ino = 0; ino < info->myrwfs_sb.entry_count; ino++) {
       
        if (read_entry_from_myrwfs(info, ino, fe) < 0)
            return -1;
       
        if (!fe->name[0])
            continue;
       
        if (strcmp(fe->name, fn) == 0)
            return MYRWFS_TO_VFS_INODE_NUM(ino);
    }

    return -1;
}


int fill_myrwfs_info(myrwfs_info_t *info) {

    if (read_sb_from_myrwfs(info, &info->myrwfs_sb) < 0) {
        return -1;
    }

    if (info->myrwfs_sb.magic_number != MYRWFS_MAGIC_NUMBER) {
        printk(KERN_ERR "Invalid MYRWFS detected. Giving up.\n");
        return -1;
    }

    loff_t *used_blocks = (loff_t *)(vmalloc(info->myrwfs_sb.bdev_size_blocks));
    if (!used_blocks) {
        return -ENOMEM;
    }

    int i;
    for (i = 0; i < info->myrwfs_sb.data_block_start; i++) {
        used_blocks[i] = 1;
    }
    for (; i < info->myrwfs_sb.bdev_size_blocks; i++) {
        used_blocks[i] = 0;
    }

    myrwfs_file_entry_t fe;
    for (int i = 0; i < info->myrwfs_sb.entry_count; i++) {
        if (read_entry_from_myrwfs(info, i, &fe) < 0) {
            vfree(used_blocks);
            return -1;
        }
       
        if (!fe.name[0])
            continue;
       
        for (int j = 0; j < MYRWFS_DATA_BLOCK_CNT; j++) {
            if (fe.blocks[j] == 0) break;
            used_blocks[fe.blocks[j]] = 1;
        }
    }

    info->used_blocks = used_blocks;
    info->vfs_sb->s_fs_info = info;
    return 0;
}

void kill_mywfs_info(myrwfs_info_t *info) {
    if (info->used_blocks)
        vfree(info->used_blocks);
}



int myrwfs_get_data_block(myrwfs_info_t *info) {
    int i;

    for (i = info->myrwfs_sb.data_block_start; i < info->myrwfs_sb.bdev_size_blocks; i++) {
        if (info->used_blocks[i] == 0) {
            info->used_blocks[i] = 1;
            return i;
        }
    }
    return -1;
}

void myrwfs_put_data_block(myrwfs_info_t *info, int i) {
    info->used_blocks[i] = 0;
}



static int myrwfs_inode_create(struct user_namespace *k, struct inode *parent_inode, struct dentry *dentry, umode_t mode, bool excl) {
    char fn[dentry->d_name.len + 1];
    int perms = 0;
    myrwfs_info_t *info = (myrwfs_info_t *)(parent_inode->i_sb->s_fs_info);
    int ino;
    struct inode *file_inode;
    myrwfs_file_entry_t fe;

    printk(KERN_INFO "** MYRWFS: myrwfs_inode_create\n");

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
    if ((ino = myrwfs_create(info, fn, perms, &fe)) == -1)
        return -1;

    file_inode = new_inode(parent_inode->i_sb);
    if (!file_inode)
    {
        myrwfs_remove(info, fn);
        return -ENOMEM;
    }
    printk(KERN_INFO "** MYRWFS: Created new VFS inode for #%d, let's fill in\n", ino);
    file_inode->i_ino = ino;
    file_inode->i_size = fe.size;
    file_inode->i_mode = S_IFREG | mode;
    file_inode->i_fop = &myrwfs;
    if (insert_inode_locked(file_inode) < 0)
    {
        make_bad_inode(file_inode);
        iput(file_inode);
        myrwfs_remove(info, fn);
        return -1;
    }
    d_instantiate(dentry, file_inode);
    unlock_new_inode(file_inode);

    return 0;
}

static struct dentry *myrwfs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned int flags) {
    myrwfs_info_t *info = (myrwfs_info_t *)(parent_inode->i_sb->s_fs_info);
    char fn[dentry->d_name.len + 1];
    int ino;
    myrwfs_file_entry_t fe;
    struct inode *file_inode = NULL;

    printk(KERN_INFO "** MYRWFS: myrwfs_inode_lookup\n");

    if (parent_inode->i_ino != myrwfs_root_inode->i_ino)
        return ERR_PTR(-ENOENT);z

    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;
    if ((ino = myrwfs_lookup(info, fn, &fe)) == -1)
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
        file_inode->i_fop = &myrwfs;
        unlock_new_inode(file_inode);
    }

    d_add(dentry, file_inode);
    return NULL;
}

static struct inode_operations myrwfs_iops = {
    create: myrwfs_inode_create,
    lookup: myrwfs_inode_lookup
};


























