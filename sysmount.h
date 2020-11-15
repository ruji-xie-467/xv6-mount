#define NMNT 10

struct mntent{
  uint devno;
  struct inode * mnti;
  uint refcnt;
  struct mntent * next;
};

struct {
  struct spinlock lock;           // guard ref count and pmntlist
  struct mntent gmnttable[NMNT];
  struct mntent * pmntlist; // per process for namespace, p stands for process
  struct mntent * prootmnt;  // per process for namespace, never change after initialization
} gmnt; // global data structure for mount