#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/pagemap.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>


MODULE_LICENSE("GPL");


#define S2FS_MAGIC_NUMBER 0x13131314
#define SLABNAME                "my_cache"

struct s2fs_inode
{
    int i_mode;
    unsigned long i_ino;
} s2fs_inode;


static int sco = 0;
static int number = 31;
static void **line = NULL;
struct kmem_cache *cache = NULL; 
static int size = sizeof(struct s2fs_inode);

void co(void* p)
{ 
    *(int*)p = (int)p; 
    sco++; 
} 


// struct list_head {
// 	struct list_head *next, *prev;
// };

static inline int is_head(const struct list_head *list, const struct list_head *head)
{
	return list == head;
}

// прототипы функий
static struct dentry *s2fs_mount(struct file_system_type *type, int flags, char const *dev, void *data);
static struct inode *s2fs_make_inode(struct super_block *sb, int mode);
static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name);
static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *dir, const char *file_name, int pid);

// структура описывающая файловую систему
static struct file_system_type s2fs_type = {
    .owner = THIS_MODULE,
    .name = "s2fs",
    .mount = s2fs_mount,
    .kill_sb = kill_litter_super,
    .fs_flags = FS_USERNS_MOUNT,
};

// поиск родителя всех (то есть поиск root)
struct task_struct *find_root(void)
{
    struct task_struct *root = current;
    while (root->pid != 0)
    {
        root = root->parent;
    }
    return root;
}

// /**
//  * list_empty - tests whether a list is empty
//  * @head: the list to test.
//  */
// static inline int list_empty(const struct list_head *head)
// {
// 	return READ_ONCE(head->next) == head;
// }

// создание файлов и папок для всех процессов
static void print_process(struct task_struct *task, struct super_block *sb, struct dentry *parent)
{
    struct list_head *list;
    struct task_struct *cur;
    char buff[256];

    if (task->pid == 0)
    {
        sprintf(buff, "%d", task->pid);
        parent = s2fs_create_dir(sb, parent, buff);
        s2fs_create_file(sb, parent, buff, task->pid);
    }

    // Использование list_for_each для дочерних элементов означает, что дочерние элементы являются корнем. 
    // Использование list_entry для sibling означает, что sibling является записью списка.
    // list_for_each(list, &task->children)
    for (list = (&task->children)->next; !is_head(list, (&task->children)); list = list->next)
    {
        cur = list_entry(list, struct task_struct, sibling);
        sprintf(buff, "%d", cur->pid);
        if (!list_empty(&cur->children))
        {
            struct dentry *new_parent = s2fs_create_dir(sb, parent, buff);
            s2fs_create_file(sb, new_parent, buff, cur->pid);
            print_process(cur, sb, new_parent); // создание каталога и рекурсивный вызов print_process
        }
        else
        {
            s2fs_create_file(sb, parent, buff, cur->pid);
        }
    }
}

// функция, выполняющая уничтожение структуры суперблок пространства ядра
static void s2fs_put_super(struct super_block *sb)
{
    printk(">>> s2fs super block destroyed\n");
}

// операции над супер блоком
static struct super_operations const s2fs_super_ops = {
    .put_super = s2fs_put_super,
};


// typedef struct {
// 	uid_t val;
// } kuid_t;


// typedef struct {
// 	gid_t val;
// } kgid_t;

// #define KUIDT_INIT(value) (kuid_t){ value }
// #define KGIDT_INIT(value) (kgid_t){ value }

// функция для создания индекса, принимает суперблок файловой системы
// Mode определяет, является ли индекс файлом или каталогом, и биты разрешений
static struct inode *s2fs_make_inode(struct super_block *sb, int mode)
{
    struct inode *entry = new_inode(sb);

    if (entry)
    {
        entry->i_ino = get_next_ino();
        entry->i_size = PAGE_SIZE;
        entry->i_mode = mode;
        entry->i_uid = KUIDT_INIT(0);
        entry->i_gid = KGIDT_INIT(0);
        entry->i_blocks = 0;
        entry->i_private = &s2fs_inode;
        entry->i_atime = entry->i_mtime = entry->i_ctime = current_time(entry);
    }

    printk(KERN_INFO ">>> s2fs_make_inode\n");

    return entry;
}

// Операции с файлами

// функция для открытия файла копирует свойства файла в свойства inode
static int s2fs_open(struct inode *ino, struct file *fp)
{
    fp->private_data = ino->i_private;
    return 0;
}

// функция для получения информации о задаче и печати ее в буфер
int get_task_info(int pid, char *data)
{
    struct pid *pidp = find_get_pid((pid_t)pid);
    struct task_struct *task = get_pid_task(pidp, PIDTYPE_PID);
    if (task == NULL)
        return -1;
    
    
    sprintf(data, "Name: %s\nState: %ld\nPID: %d\nCPU_ID: %d\n\
TGID: %d\nPPID: %d\nStart_Time: %llu\n\
Dynamic_Priority: %d\nStatic_Priority: %d\n\
Normal_Priority: %d\nReal_Time_Priority: %d\n\
Memory_Map_Base: %lu\nVirtual_Memory_Space: %lu\n\
Virtual_Memory_Usage: %lu\nVirtual_Memory_Num: %d\nPages_Mapped: %lu\n", 
          task->comm, task->state, task->pid, task->cpu, task->tgid, task->real_parent->pid,
          task->start_time, task->prio, task->static_prio, task->normal_prio, task->rt_priority, 
          task->mm->mmap_base, task->mm->task_size, task->mm->highest_vm_end, task->mm->map_count, task->mm->total_vm); 
  return strlen(data);
}

// функция для получения pid из указателя файла
int get_pid_from_file(struct file *fp) //
{
    int pid;
    char *file_name = fp->f_path.dentry->d_iname;
    sscanf(file_name, "%d", &pid);

    return pid;
}

#define TMP_SIZE 1024
// функция для чтения файла
static ssize_t s2fs_read_file(struct file *fp, char *buf, size_t count, loff_t *offset)
{
    int len;
    char tmp[TMP_SIZE];
    int pid = get_pid_from_file(fp);
    len = get_task_info(pid, tmp);

    if (len == -1)
        len = snprintf(tmp, TMP_SIZE, "Task no longer exists.\n");

    if (*offset > len)
        return 0;

    if (count > len - *offset)
        count = len - *offset;

    if (copy_to_user(buf, tmp, count))
        return -EFAULT;

    *offset += count;
    return count;
}

// функция записи в файл
static ssize_t s2fs_write_file(struct file *fp, const char *buf, size_t count, loff_t *offset)
{
    atomic_t *counter = (atomic_t *)fp->private_data;
    char tmp[TMP_SIZE];

    // Пишем только с самого начала
    if (*offset != 0)
        return -EINVAL;

    // Чтение от пользователя
    if (count >= TMP_SIZE)
        return -EINVAL;

    memset(tmp, 0, TMP_SIZE);
    if (copy_from_user(tmp, buf, count))
        return -EFAULT;

    atomic_set(counter, simple_strtol(tmp, NULL, 10));
    return count;
}

// файловые операции для использования вышеуказанных функций
static struct file_operations s2fs_fops = {
    .open = s2fs_open,
    .read = s2fs_read_file,
    .write = s2fs_write_file,
};


// /*
//  * "quick string" -- eases parameter passing, but more importantly
//  * saves "metadata" about the string (ie length and the hash).
//  *
//  * hash comes first so it snuggles against d_parent in the
//  * dentry.
//  */
// struct qstr {
// 	union {
// 		struct {
// 			HASH_LEN_DECLARE;
// 		};
// 		u64 hash_len;
// 	};
// 	const unsigned char *name;
// };

// unsigned int full_name_hash(const void *salt, const char *name, unsigned int len)
// Возвращает хэш строки известной длины.

// функция для создания файла dentry и inode
static struct dentry *s2fs_create_file(struct super_block *sb, struct dentry *dir, const char *file_name, int pid)
{
    struct dentry *ret;
    struct inode *ino;
    struct qstr qname;

    // задайте свойства имени файла
    qname.name = file_name;
    qname.len = strlen(file_name);
    qname.hash = full_name_hash(dir, file_name, qname.len);
    
    // выделение запись с именем файла
    ret = d_alloc(dir, &qname);
    if (!ret)
        return 0;
    
    // выделение inode для файла
    ino = s2fs_make_inode(sb, S_IFREG | 0644);
    if (!ino)
    {
        // void dput(struct dentry *dentry)
        // Release a dentry.
        dput(ret);
        return 0;
    }
    
    // задайте файловые операции с inode
    ino->i_fop = &s2fs_fops;
    ino->i_private = &pid;
    
    // ino->i_private
    // добавьте пару entry и ino
    // ПРО void d_add(struct dentry *, struct inode *);
    //  * This adds the entry to the hash queues and initializes @inode.
    //  * The entry was actually filled in earlier during d_alloc().
    d_add(ret, ino);
    return ret;
}

// функция для создания каталога dentry и inode
static struct dentry *s2fs_create_dir(struct super_block *sb, struct dentry *parent, const char *dir_name)
{
    struct dentry *ret = NULL;
    struct inode *ino = NULL;
    struct qstr qname;

    // заполнение параметров имени каталога
    qname.name = dir_name;
    qname.len = strlen(dir_name);
    qname.hash = full_name_hash(parent, dir_name, qname.len);
    ret = d_alloc(parent, &qname);
    if (!ret)
        return 0;

    ino = s2fs_make_inode(sb, S_IFDIR | 0755);
    if (!ino)
    {
        dput(ret);
        return 0;
    }

    // set inode operations
    ino->i_op = &simple_dir_inode_operations;
    ino->i_fop = &simple_dir_operations;

    d_add(ret, ino);

    return ret;
}

// функция для создания sb, необходимого для монтирования файловой системы
static int s2fs_fill_sb(struct super_block *sb, void *data, int silent)
{
    struct inode *root = NULL;
    struct dentry *root_dentry;

    // установка параметров суперблока
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = S2FS_MAGIC_NUMBER;
    sb->s_op = &s2fs_super_ops;

    // создание индекса для корневого каталога
    root = s2fs_make_inode(sb, S_IFDIR | 0755);
    if (!root)
    {
        printk (KERN_ERR ">>> S2FS inode allocation failed!\n");
        return -ENOMEM;
    }

    root->i_op = &simple_dir_inode_operations;
    root->i_fop = &simple_dir_operations;

    // создание и установка корневую запись для суперблока
    root_dentry = d_make_root(root);
    if (!root_dentry)
    {
        iput(root);
        return -ENOMEM;
    }
    sb->s_root = root_dentry;

    print_process(find_root(), sb, root_dentry);

    return 0;
}

// функция для определения способа монтирования файловой системы
static struct dentry *s2fs_mount(struct file_system_type *type, int flags, char const *dev, void *data)
{
    struct dentry *const entry = mount_nodev(type, flags, data, s2fs_fill_sb);
    
    if (IS_ERR(entry))
        printk(KERN_ERR ">>> S2FS mounting failed !\n") ;
    else
        printk(KERN_DEBUG ">>> S2FS mounted!\n") ;

    return entry;
}


static int __init s2fs_init(void)
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
        
    ret = register_filesystem(&s2fs_type);
    if (ret)
    {
        printk(KERN_ERR ">>> S2FS_MODULE can not register filesystem!\n");
        
        kmem_cache_destroy(cache); 
        kfree(line);

        return ret;
    }

    printk(KERN_INFO ">>> S2FS_MODULE loaded!\n");

    printk(KERN_INFO ">>> allocate %d objects into slab: %s\n", number, SLABNAME); 
	printk(KERN_INFO ">>> object size %d bytes, full size %ld bytes\n", size, (long)size * number); 
	printk(KERN_INFO ">>> constructor called %d times\n", sco); 

    return 0;
}

static void __exit s2fs_exit(void)
{
    kmem_cache_free(cache, *line);
    kmem_cache_destroy(cache); 
    kfree(line);
    
    if (unregister_filesystem(&s2fs_type))
        printk(KERN_ERR ">>> S2FS_MODULE can not unregister filesystem!\n");

    printk(KERN_INFO ">>> S2FS_MODULE unloaded!\n");
}

module_init(s2fs_init);
module_exit(s2fs_exit);
