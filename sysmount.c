#include "types.h"
#include "defs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "param.h"
#include "stat.h"
#include "sysmount.h"


void mountinit(void) {
  struct mntent * rootmnt = &(gmnt.gmnttable[0]);
  rootmnt->devno = 0;
  rootmnt->mnti = 0;
  rootmnt->refcnt = 1;
  gmnt.prootmnt = rootmnt;
  gmnt.pmntlist = rootmnt;

  initlock(&gmnt.lock, "gmnt");

}

struct mntent * mntalloc() {
  acquire(&gmnt.lock);
  struct mntent * newmntent = 0;
  for (int i = 0; i < NMNT; i++) {
    if (gmnt.gmnttable[i].refcnt == 0) {
      newmntent = &gmnt.gmnttable[i];
      break;
    }
  }
  if (newmntent == 0) {
    panic("[mntalloc] no free mount entry");
  }
  newmntent->refcnt = 2;
  release(&gmnt.lock);
  return newmntent;
}

struct mntent * mntdup(struct mntent * mntent) {
  // cprintf("mntdup %d...\n", (int) mntent->devno);
  acquire(&gmnt.lock);
  mntent->refcnt++;
  release(&gmnt.lock);
  return mntent;
}

void mntput(struct mntent * mntent) {
  cprintf("mntput %d, ref: %d...\n", (int) mntent->devno, (int) mntent->refcnt);
  acquire(&gmnt.lock);
  mntent->refcnt--;
  release(&gmnt.lock);
}

int sys_mount(void) {
  char *mntpnt_path, *dev_path;

  if(argstr(0, &dev_path) < 0 || argstr(1, &mntpnt_path) < 0) {
    return -1;
  }

  begin_op();

  struct inode * mnti = namei(mntpnt_path);
  struct inode * devi = namei(dev_path);

  if (mnti == 0) {
    panic("[sys_mount] mountpoint doesn't exist");
    // end_op();
    // return -1;
  }

  if (devi == 0) {
    panic("[sys_mount] loop device doesn't exist");
    // end_op();
    // return -1;
  }

  ilock(mnti);
  ilock(devi);

  if (mnti->inum == ROOTINO) {
    cprintf("[sys_mount] mountpoint corresponds to be root inode\n.");
    // panic("[sys_mount] mountpoint corresponds to be root inode");
    iunlockput(mnti);
    iunlockput(devi);
    end_op();
    return -1;
  }

  if (devi->type != T_FILE) {
    cprintf("[sys_mount] device is not a file (only support loop divice for now)");
    iunlockput(mnti);
    iunlockput(devi);
    end_op();
    return -1;
  }

  uint devno = getorcreatedev(devi);
  struct mntent * newmntent = mntalloc();

  newmntent->devno = devno;
  newmntent->mnti = idup(mnti);

  // add to pmnt list
  newmntent->next = gmnt.pmntlist;
  gmnt.pmntlist = newmntent;

  mntput(newmntent);
  iunlockput(mnti); // newmntent holds a reference
  iunlockput(devi);
  // iunlock(mnti); // newmntent holds a reference
  // iunlock(devi);
  end_op();
  return 0;

}

int sys_umount(void) {
  char *mntpnt_path;

  if(argstr(0, &mntpnt_path) < 0) {
    return -1;
  }

  begin_op();
  cprintf("we will umount %s\n", mntpnt_path);
  struct inode * mnti = namei(mntpnt_path);


  // struct inode * devi = namei(dev_path);

  if (mnti == 0) {
    panic("[sys_mount] mountpoint doesn't exist");
    // end_op();
    // return -1;
  }

  ilock(mnti);
  if (mnti->dev == ROOTDEV) {
    cprintf("[sys_umount] unmount root device\n");
    iunlock(mnti);
    end_op();
    return -1;
  }
  
  if (mnti->inum != ROOTINO) {
    cprintf("[sys_umount] unmount non-root node\n");
    iunlock(mnti);
    end_op();
    return -1;
  }
  iunlock(mnti);

  
  struct mntent * m = 0;
  acquire(&gmnt.lock); 
  for (struct mntent * cur = gmnt.pmntlist; cur != 0; cur = cur->next) {
    if (cur->refcnt == 0) {
      panic("[mntlookup] mnt entry in active mount list has a zero refcnt");
    }
    if (cur->devno == mnti->dev) { 
      m = cur;
      break;
    }
  }

  if (m == 0) {
    cprintf("[sys_umount] %s doesn't mount to any node\n", mntpnt_path);
    release(&gmnt.lock); 
    end_op();
    return -1;
  }

  if (m->refcnt > 1) {
    cprintf("[sys_umount] %s is busy, cannot unmount it\n", mntpnt_path);
    release(&gmnt.lock); 
    end_op();
    return -1;
  }
  
  struct mntent * cur = gmnt.pmntlist;
  struct mntent ** pre = &gmnt.pmntlist;

  while (cur != 0) {
    if (cur == m) {
      break;
    }
    pre = &cur->next;
    cur = cur->next;
  }

  if (cur == 0) {
    panic("[mntlookup] mnt entry is not in active mount list");
  }

  *pre = cur->next;
  release(&gmnt.lock); 

  cur->devno = 0;
  cur->mnti = 0;
  cur->next = 0;
  cur->refcnt = 0;

  struct inode * devi = getlloopdevi(cur->devno);
  iput(devi);
  iput(mnti);
  cprintf("devi->ref: %d\n", devi->ref);
  cprintf("mnti->ref: %d\n", mnti->ref);
  end_op();
  return 0;

}

// mount point to mntent
struct mntent * mntlookup(struct inode * ip) {
  
  acquire(&gmnt.lock);
  for (struct mntent * cur = gmnt.pmntlist; cur != 0; cur = cur->next) {
    if (cur->refcnt == 0) {
      panic("[mntlookup] mnt entry in active mount list has a zero refcnt");
    }
    // if (cur->mnti == 0) {
    //   panic("[mntlookup] mountpoint inode is 0");
    // }
    if (cur->mnti == ip) { 
      release(&gmnt.lock);
      return mntdup(cur);
    }
  }

  release(&gmnt.lock);
  return 0;

}


// mount device to mount point
struct inode * getmntpnt(uint devno) {
  acquire(&gmnt.lock);
  for (struct mntent * cur = gmnt.pmntlist; cur != 0; cur = cur->next) {
    if (cur->refcnt == 0) {
      panic("[mntlookup] mnt entry in active mount list has a zero refcnt");
    }
    // if (cur->mnti == 0) {
    //   panic("[mntlookup] mountpoint inode is 0");
    // }
    if (cur->devno == devno) { 
      release(&gmnt.lock);
      return cur->mnti;
    }
  }
  release(&gmnt.lock);
  return 0;
}