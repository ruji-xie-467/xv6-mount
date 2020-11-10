#include "types.h"
#include "defs.h"
#include "spinlock.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "buf.h"
#include "loopdev.h"

// TODO: persist to disk
struct {
  struct spinlock lock;
  struct loopdev loopdevtable[NLOOPDEV]; // array idx is loopdevno
} gloopdev;

int isloopdev(uint devno) {
  uint loopdevbit = (devno >> 5) & 1;
  return loopdevbit;
}

void loopdevinit(void) {
  initlock(&gloopdev.lock, "loopdev");
}

struct inode * getlloopdevi(uint devno) {
  uint loopdevno = devno & (~LOOPDEV_MASK);
  if (loopdevno >= NLOOPDEV) {
    panic("[getloopdevi] invalid loop device number");
  }
  return gloopdev.loopdevtable[loopdevno].ip;
}

uint getorcreatedev(struct inode * ip) {
  acquire(&gloopdev.lock);
  struct loopdev * freeloopdev = 0;
  uint freeloopdevno = 0;
  for (uint i = 0; i < NLOOPDEV; i++) {
    if (!freeloopdev && !gloopdev.loopdevtable[i].used) {
      freeloopdev = &gloopdev.loopdevtable[i];
      freeloopdevno = i;
    }
    if (gloopdev.loopdevtable[i].used && gloopdev.loopdevtable[i].ip == ip) {
      release(&gloopdev.lock);
      return i | LOOPDEV_MASK;
    }
  }
  if (freeloopdev == 0) {
    panic("[getorcreatedev] no free loop device");
  }
  freeloopdev->used = 1;
  release(&gloopdev.lock);

  freeloopdev->ip = idup(ip);
  return freeloopdevno | LOOPDEV_MASK;
}

void devput(uint devno) {
  if (!isloopdev(devno)) {
    panic("[devput] devno is not a loop device");
  }
  uint loopdevno = devno & (~LOOPDEV_MASK);
  acquire(&gloopdev.lock);
  gloopdev.loopdevtable[loopdevno].used = 0;  
  release(&gloopdev.lock);
}

struct buf* loopdev_read(struct buf* b) {
  uint devno = b->dev; 
  uint blockno = b->blockno;
  struct inode * ip = getlloopdevi(devno);
  ilock(ip);
  int res = readi(ip, (char*) b->data, blockno * BSIZE, BSIZE);
  iunlock(ip);
  if (res != BSIZE) {
    cprintf("res: %d \n", res);
    // panic("[loopdev_read] loopdev_read didn't read a whole block");
  }
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  return b;
}

void loopdev_write(struct buf* b) {
  uint devno = b->dev; 
  uint blockno = b->blockno;
  struct inode * ip = getlloopdevi(devno);
  writei(ip, (char*) b->data, blockno * BSIZE, BSIZE);
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
}


