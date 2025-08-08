/*
 * kernel_stub.h - Generic Linux kernel stubs for Joern CPG parsing
 * 
 * This file provides minimal type definitions, macros, and function declarations
 * to satisfy the Joern parser when processing Linux kernel code snippets.
 * It is NOT meant for compilation, only for parsing to avoid flat CPGs.
 */

#ifndef KERNEL_STUB_H
#define KERNEL_STUB_H

// ===== BASIC TYPES =====
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed char        s8;
typedef signed short       s16;
typedef signed int         s32;
typedef signed long long   s64;
typedef unsigned long      ulong;
typedef _Bool              bool;
typedef unsigned long      size_t;
typedef long               ssize_t;
typedef int                gfp_t;
typedef void*              dma_addr_t;

// ===== CONSTANTS =====
#define true  1
#define false 0
#define NULL  ((void*)0)

// ===== KERNEL ANNOTATIONS =====
#define __user
#define __kernel
#define __iomem
#define __init
#define __exit
#define __initdata
#define __devinit
#define __devexit
#define __attribute__(x)
#define __packed
#define __aligned(x)
#define __maybe_unused
#define __always_inline
#define __force
#define __bitwise
#define __rcu
#define __percpu
#define __read_mostly
#define __cold
#define __hot
#define __weak
#define __section(x)
#define __visible
#define __printf(a,b)
#define __scanf(a,b)
#define __noreturn
#define __pure
#define __const
#define __must_check
#define __deprecated
#define __used
#define __unused
#define __noinline
#define __cacheline_aligned
#define likely(x)     (x)
#define unlikely(x)   (x)
#define barrier()
#define smp_mb()
#define smp_rmb()
#define smp_wmb()

// ===== MEMORY MANAGEMENT =====
#define GFP_KERNEL    0
#define GFP_ATOMIC    1
#define GFP_USER      2
#define GFP_NOWAIT    4
#define __GFP_WAIT    8
#define VERIFY_READ   0
#define VERIFY_WRITE  1

// ===== ERROR CODES =====
#define EPERM         1
#define ENOENT        2
#define ESRCH         3
#define EINTR         4
#define EIO           5
#define ENXIO         6
#define E2BIG         7
#define ENOEXEC       8
#define EBADF         9
#define ECHILD       10
#define EAGAIN       11
#define ENOMEM       12
#define EACCES       13
#define EFAULT       14
#define ENOTBLK      15
#define EBUSY        16
#define EEXIST       17
#define EXDEV        18
#define ENODEV       19
#define ENOTDIR      20
#define EISDIR       21
#define EINVAL       22
#define ENFILE       23
#define EMFILE       24
#define ENOTTY       25
#define ETXTBSY      26
#define EFBIG        27
#define ENOSPC       28
#define ESPIPE       29
#define EROFS        30
#define EMLINK       31
#define EPIPE        32
#define EDOM         33
#define ERANGE       34

// ===== KERNEL LOG LEVELS =====
#define KERN_EMERG    "<0>"
#define KERN_ALERT    "<1>"
#define KERN_CRIT     "<2>"
#define KERN_ERR      "<3>"
#define KERN_WARNING  "<4>"
#define KERN_NOTICE   "<5>"
#define KERN_INFO     "<6>"
#define KERN_DEBUG    "<7>"

// ===== COMMON KERNEL STRUCTURES (FORWARD DECLARATIONS) =====
struct task_struct;
struct file;
struct inode;
struct dentry;
struct super_block;
struct address_space;
struct page;
struct mm_struct;
struct vm_area_struct;
struct sk_buff;
struct net_device;
struct pci_dev;
struct device;
struct kobject;
struct kref;
struct work_struct;
struct timer_list;
struct hlist_head;
struct hlist_node;
struct list_head;
struct rcu_head;
struct mutex;
struct semaphore;
struct rwsem;
struct spinlock;
struct raw_spinlock;
struct wait_queue_head;
struct completion;
struct notifier_block;
struct seq_file;
struct proc_dir_entry;
struct sysfs_dirent;
struct attribute;
struct bin_attribute;
struct attribute_group;
struct kset;
struct ktype;
struct subsystem;

// ===== COMMON KERNEL VARIABLES (EXTERN) =====
extern int console_loglevel;
extern int minimum_console_loglevel;
extern int default_message_loglevel;
extern u64 log_first_seq, log_next_seq, syslog_seq;
extern u32 log_first_idx, log_next_idx, syslog_idx;
extern int log_buf_len;
extern struct wait_queue_head log_wait;
extern struct mutex log_mutex;
extern raw_spinlock_t logbuf_lock;

// ===== COMMON KERNEL FUNCTIONS (DUMMY DECLARATIONS) =====
// Memory management
void* kmalloc(size_t size, gfp_t flags);
void* kzalloc(size_t size, gfp_t flags);
void* vmalloc(unsigned long size);
void kfree(const void *ptr);
void vfree(const void *ptr);
int access_ok(int type, const void *addr, unsigned long size);
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);

// Locking
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);
void spin_lock_irqsave(spinlock_t *lock, unsigned long flags);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);
void raw_spin_lock_irq(raw_spinlock_t *lock);
void raw_spin_unlock_irq(raw_spinlock_t *lock);
void mutex_lock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);
int mutex_trylock(struct mutex *lock);

// Wait queues
int wait_event_interruptible(struct wait_queue_head wq, int condition);
int wait_event_timeout(struct wait_queue_head wq, int condition, long timeout);
void wake_up(struct wait_queue_head *q);
void wake_up_interruptible(struct wait_queue_head *q);

// Logging
int printk(const char *fmt, ...);
int pr_info(const char *fmt, ...);
int pr_err(const char *fmt, ...);
int pr_warn(const char *fmt, ...);
int pr_debug(const char *fmt, ...);

// Security
int check_syslog_permissions(int type, bool from_file);
int security_syslog(int type);
int capable(int cap);

// Syslog specific
int syslog_print(char *buf, int len);
int syslog_print_all(char *buf, int len, bool clear);
int syslog_print_line(u32 idx, char *dst, int maxlen);
u32 log_next(u32 idx);

// Process management
struct task_struct *current;
pid_t task_pid_nr(struct task_struct *tsk);
uid_t task_uid(struct task_struct *task);
gid_t task_gid(struct task_struct *task);

// File system
int generic_permission(struct inode *inode, int mask);
struct dentry *d_find_alias(struct inode *inode);
void d_drop(struct dentry *dentry);
void iput(struct inode *inode);

// Network
int netif_rx(struct sk_buff *skb);
void kfree_skb(struct sk_buff *skb);
struct sk_buff *alloc_skb(unsigned int size, gfp_t priority);

// Time
unsigned long jiffies;
unsigned long get_jiffies_64(void);

// Module support
int __init init_module(void);
void __exit cleanup_module(void);

// Common macros
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define BUG() do { } while (0)
#define BUG_ON(condition) do { if (condition) BUG(); } while (0)
#define WARN_ON(condition) (condition)
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

// Bit operations
#define set_bit(nr, addr) do { } while (0)
#define clear_bit(nr, addr) do { } while (0)
#define test_bit(nr, addr) 0
#define test_and_set_bit(nr, addr) 0
#define test_and_clear_bit(nr, addr) 0

#endif /* KERNEL_STUB_H */
