/*
 * kernel_stub.h - Unified Linux kernel stubs for Joern CPG parsing
 * 
 * This file provides comprehensive type definitions, macros, and function declarations
 * to satisfy the Joern parser when processing Linux kernel code snippets.
 * 
 * FUSION DE TOUTES LES VERSIONS:
 * - kernel_stub_improved.h (structures détaillées et fonctions complètes)
 * - kernel_stub_backup.h (éléments de base originaux)
 * - kernel_stub_gcc_generated.h (macro IS_MODULE détectée par analyse GCC)
 * 
 * It is NOT meant for compilation, only for parsing to avoid flat CPGs.
 */

#ifndef KERNEL_STUB_H
#define KERNEL_STUB_H

// ===== BASIC TYPES =====
typedef unsigned char      u8, __u8;
typedef unsigned short     u16, __u16;
typedef unsigned int       u32, __u32;
typedef unsigned long long u64, __u64;
typedef signed char        s8, __s8;
typedef signed short       s16, __s16;
typedef signed int         s32, __s32;
typedef signed long long   s64, __s64;
typedef unsigned long      ulong;
typedef _Bool              bool;
typedef unsigned long      size_t;
typedef long               ssize_t;
typedef int                gfp_t;
typedef void*              dma_addr_t;
typedef int                pid_t;
typedef int                uid_t;
typedef int                gid_t;
typedef long               loff_t;

// Standard C types often missing
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
typedef unsigned long long uint64_t;
typedef int                int32_t;
typedef short              int16_t;
typedef char               int8_t;
typedef long long          int64_t;

// ===== CONSTANTS =====
#define true  1
#define false 0
#define NULL  ((void*)0)

// ===== MODULE DETECTION (FROM GCC ANALYSIS) =====
#define IS_MODULE 1  // Detecté automatiquement par analyse GCC (2 occurrences)

// ===== KERNEL ANNOTATIONS AND ATTRIBUTES =====
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
#define __must_hold(x)
#define __releases(x)
#define __acquires(x)
#define __exclusive_lock_function(x)
#define __exclusive_unlock_function(x)
#define __shared_lock_function(x)
#define __shared_unlock_function(x)
#define likely(x)     (x)
#define unlikely(x)   (x)
#define barrier()
#define smp_mb()
#define smp_rmb()
#define smp_wmb()

// ===== MEMORY MANAGEMENT FLAGS =====
#define GFP_KERNEL    0
#define GFP_ATOMIC    1
#define GFP_USER      2
#define GFP_NOWAIT    4
#define GFP_NOIO      8
#define GFP_NOFS      16
#define GFP_DMA       32
#define GFP_DMA32     64
#define __GFP_WAIT    128
#define __GFP_HIGH    256
#define __GFP_IO      512
#define __GFP_FS      1024
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
#define EOPNOTSUPP   95

// ===== FILE AND I/O FLAGS =====
#define O_RDONLY     0
#define O_WRONLY     1
#define O_RDWR       2
#define O_CREAT      64
#define O_EXCL       128
#define O_NOCTTY     256
#define O_TRUNC      512
#define O_APPEND     1024
#define O_NONBLOCK   2048
#define O_SYNC       4096
#define O_CLOEXEC    524288

// ===== CAPABILITIES =====
#define CAP_SYS_ADMIN    21
#define CAP_SYS_RESOURCE 24
#define CAP_NET_ADMIN    12
#define CAP_DAC_OVERRIDE 1

// ===== KERNEL LOG LEVELS =====
#define KERN_EMERG    "<0>"
#define KERN_ALERT    "<1>"
#define KERN_CRIT     "<2>"
#define KERN_ERR      "<3>"
#define KERN_WARNING  "<4>"
#define KERN_NOTICE   "<5>"
#define KERN_INFO     "<6>"
#define KERN_DEBUG    "<7>"

// ===== COMMON STRUCTURES (DETAILED DEFINITIONS) =====
struct list_head {
    struct list_head *next, *prev;
};

struct hlist_head {
    struct hlist_node *first;
};

struct hlist_node {
    struct hlist_node *next, **pprev;
};

struct fd {
    struct file *file;
    unsigned int flags;
};

struct msghdr {
    void *msg_name;
    int msg_namelen;
    struct iovec *msg_iov;
    size_t msg_iovlen;
    void *msg_control;
    size_t msg_controllen;
    unsigned int msg_flags;
};

struct iovec {
    void *iov_base;
    size_t iov_len;
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
};

struct in6_addr {
    union {
        __u8 u6_addr8[16];
        __u16 u6_addr16[8];
        __u32 u6_addr32[4];
    } in6_u;
};

struct in6_pktinfo {
    struct in6_addr ipi6_addr;
    int ipi6_ifindex;
};

struct ipv6_opt_hdr {
    __u8 nexthdr;
    __u8 hdrlen;
};

struct group_req {
    __u32 gr_interface;
    struct sockaddr_storage gr_group;
};

struct group_source_req {
    __u32 gsr_interface;
    struct sockaddr_storage gsr_group;
    struct sockaddr_storage gsr_source;
};

struct sockaddr_storage {
    unsigned short ss_family;
    char __data[128 - sizeof(unsigned short)];
};

struct flowi6 {
    struct flowi_common __fl_common;
    struct in6_addr daddr;
    struct in6_addr saddr;
    __be32 flowlabel;
    union flowi_uli uli;
};

struct flowi_common {
    int flowic_oif;
    int flowic_iif;
    __u32 flowic_mark;
    __u8 flowic_tos;
    __u8 flowic_scope;
    __u8 flowic_proto;
    __u8 flowic_flags;
    __u32 flowic_secid;
    struct flowi_tunnel flowic_tun_key;
    __u32 flowic_uid;
};

struct flowi_tunnel {
    __be64 tun_id;
};

union flowi_uli {
    struct {
        __be16 sport;
        __be16 dport;
    } ports;
    struct {
        __u8 type;
        __u8 code;
    } icmpt;
    __be32 spi;
    __be32 gre_key;
    struct {
        __u8 type;
    } mht;
};

struct perf_event_attr {
    __u32 type;
    __u32 size;
    __u64 config;
    __u64 sample_period;
    __u64 sample_type;
    __u64 read_format;
    __u64 disabled       : 1,
          inherit        : 1,
          pinned         : 1,
          exclusive      : 1,
          exclude_user   : 1,
          exclude_kernel : 1,
          exclude_hv     : 1,
          exclude_idle   : 1,
          mmap           : 1,
          comm           : 1,
          freq           : 1,
          inherit_stat   : 1,
          enable_on_exec : 1,
          task           : 1,
          watermark      : 1,
          precise_ip     : 2,
          mmap_data      : 1,
          sample_id_all  : 1,
          exclude_host   : 1,
          exclude_guest  : 1,
          exclude_callchain_kernel : 1,
          exclude_callchain_user   : 1,
          mmap2          : 1,
          comm_exec      : 1,
          use_clockid    : 1,
          context_switch : 1,
          write_backward : 1,
          namespaces     : 1,
          __reserved_1   : 35;
    union {
        __u32 wakeup_events;
        __u32 wakeup_watermark;
    };
    __u32 bp_type;
    union {
        __u64 bp_addr;
        __u64 config1;
    };
    union {
        __u64 bp_len;
        __u64 config2;
    };
    __u64 branch_sample_type;
    __u64 sample_regs_user;
    __u32 sample_stack_user;
    __s32 clockid;
    __u64 sample_regs_intr;
    __u32 aux_watermark;
    __u16 sample_max_stack;
    __u16 __reserved_2;
};

// ===== FORWARD DECLARATIONS OF COMPLEX STRUCTURES =====
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
struct perf_event;

// ===== GLOBAL VARIABLES =====
extern int console_loglevel;
extern int minimum_console_loglevel;
extern int default_message_loglevel;
extern u64 log_first_seq, log_next_seq, syslog_seq;
extern u32 log_first_idx, log_next_idx, syslog_idx;
extern int log_buf_len;
extern struct wait_queue_head log_wait;
extern struct mutex log_mutex;
extern raw_spinlock_t logbuf_lock;
extern char v86d_path[];  // Variable globale souvent référencée
extern unsigned long jiffies;

// ===== MEMORY MANAGEMENT FUNCTIONS =====
void* kmalloc(size_t size, gfp_t flags);
void* kzalloc(size_t size, gfp_t flags);
void* vmalloc(unsigned long size);
void* kcalloc(size_t n, size_t size, gfp_t flags);
void* krealloc(const void *p, size_t new_size, gfp_t flags);
void kfree(const void *ptr);
void vfree(const void *ptr);
int access_ok(int type, const void *addr, unsigned long size);
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
void* memset(void *s, int c, size_t count);
void* memcpy(void *dest, const void *src, size_t count);
void* memmove(void *dest, const void *src, size_t count);
int memcmp(const void *cs, const void *ct, size_t count);

// ===== ERROR HANDLING MACROS =====
#define IS_ERR(ptr) ((unsigned long)(ptr) > (unsigned long)(-1000))
#define PTR_ERR(ptr) ((long)(ptr))
#define ERR_PTR(error) ((void *)(long)(error))
#define IS_ERR_OR_NULL(ptr) (!ptr || IS_ERR(ptr))

// ===== LOCKING FUNCTIONS =====
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

// ===== WAIT QUEUES =====
int wait_event_interruptible(struct wait_queue_head wq, int condition);
int wait_event_timeout(struct wait_queue_head wq, int condition, long timeout);
void wake_up(struct wait_queue_head *q);
void wake_up_interruptible(struct wait_queue_head *q);

// ===== LOGGING AND DEBUG =====
int printk(const char *fmt, ...);
int pr_info(const char *fmt, ...);
int pr_err(const char *fmt, ...);
int pr_warn(const char *fmt, ...);
int pr_debug(const char *fmt, ...);
int dev_err(struct device *dev, const char *fmt, ...);
int dev_warn(struct device *dev, const char *fmt, ...);
int dev_info(struct device *dev, const char *fmt, ...);
int dev_dbg(struct device *dev, const char *fmt, ...);

// ===== WARNING AND DEBUGGING MACROS =====
#define WARN_ON(condition) (condition)
#define WARN_ON_ONCE(condition) (condition)
#define WARN(condition, fmt, ...) (condition)
#define WARN_ONCE(condition, fmt, ...) (condition)

// ===== SECURITY AND CAPABILITIES =====
int check_syslog_permissions(int type, bool from_file);
int security_syslog(int type);
int capable(int cap);

// ===== SYSCALL DEFINITIONS =====
#define SYSCALL_DEFINE0(name) int sys_##name(void)
#define SYSCALL_DEFINE1(name, ...) int sys_##name(__VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) int sys_##name(__VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) int sys_##name(__VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) int sys_##name(__VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) int sys_##name(__VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) int sys_##name(__VA_ARGS__)

// ===== PROCESS MANAGEMENT =====
struct task_struct *current;
pid_t task_pid_nr(struct task_struct *tsk);
uid_t task_uid(struct task_struct *task);
gid_t task_gid(struct task_struct *task);

// ===== FILE SYSTEM =====
int generic_permission(struct inode *inode, int mask);
struct dentry *d_find_alias(struct inode *inode);
void d_drop(struct dentry *dentry);
void iput(struct inode *inode);

// ===== NETWORK =====
int netif_rx(struct sk_buff *skb);
void kfree_skb(struct sk_buff *skb);
struct sk_buff *alloc_skb(unsigned int size, gfp_t priority);

// ===== LIST OPERATIONS =====
#define list_for_each_entry(pos, head, member) \
    for (pos = container_of((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = container_of(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = container_of((head)->next, typeof(*pos), member), \
         n = container_of(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = container_of(n->member.next, typeof(*n), member))

// ===== TIME =====
unsigned long get_jiffies_64(void);

// ===== MODULE SUPPORT =====
int __init init_module(void);
void __exit cleanup_module(void);

// ===== ESSENTIAL MACROS =====
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define min_t(type, x, y) ((type)(x) < (type)(y) ? (type)(x) : (type)(y))
#define max_t(type, x, y) ((type)(x) > (type)(y) ? (type)(x) : (type)(y))
#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)
#define clamp_t(type, val, lo, hi) min_t(type, max_t(type, val, lo), hi)
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) ((x) - ((x) % (y)))

// ===== BUG AND WARNING HANDLING =====
#define BUG() do { } while (0)
#define BUG_ON(condition) do { if (condition) BUG(); } while (0)
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

// ===== BIT OPERATIONS =====
#define set_bit(nr, addr) do { } while (0)
#define clear_bit(nr, addr) do { } while (0)
#define test_bit(nr, addr) 0
#define test_and_set_bit(nr, addr) 0
#define test_and_clear_bit(nr, addr) 0
#define find_first_bit(addr, size) 0
#define find_next_bit(addr, size, offset) 0

// ===== GENERIC CONDITIONAL MACROS =====
#define IS_ENABLED(config) 1
#define CONFIG_DEBUG_KERNEL 1

// ===== SOFTWARE EVENT FUNCTIONS (PERF) =====
int is_software_event(struct perf_event *event);
void perf_group_detach(struct perf_event *event);

// ===== STRING FUNCTIONS =====
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, size_t count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t count);

// ===== SYSLOG SPECIFIC FUNCTIONS (FROM ORIGINAL BACKUP) =====
int syslog_print(char *buf, int len);
int syslog_print_all(char *buf, int len, bool clear);
int syslog_print_line(u32 idx, char *dst, int maxlen);
u32 log_next(u32 idx);

#endif /* KERNEL_STUB_H */