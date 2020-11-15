#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "log.h"
#include "loopdev.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.





static void recover_from_log(void);
static void commit();

void
initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(dev, &sb);
  log.start = sb.logstart;
  log.size = sb.nlog;
  log.dev = dev;

  loop_log.start = sb.logstart;
  loop_log.size = sb.nlog;
  loop_log.dev = 0 | LOOPDEV_MASK;

  recover_from_log();
}

// Copy committed blocks from log to their home location
static void
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf);
    brelse(dbuf);
  }

}

static void
install_trans_loop(void)
{
  if (!is_loop_mounted) {
    panic("[install_trans_loop] loop dev not mounted");
  }
  int tail;

  for (tail = 0; tail < loop_log.lh.n; tail++) {
    struct buf *lbuf = bread(loop_log.dev, loop_log.start+tail+1); // read log block
    struct buf *dbuf = bread(loop_log.dev, loop_log.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    brelse(lbuf);
    brelse(dbuf);
  }

}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{

  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);

}

//// Read the log header from disk into the in-memory log header
//static void
//read_head_loop(void)
//{
//  if (!is_loop_mounted) {
//    panic("[read_head_loop] loop dev not mounted");
//  }
//  struct buf *buf = bread(loop_log.dev, loop_log.start);
//  struct logheader *lh = (struct logheader *) (buf->data);
//  int i;
//  loop_log.lh.n = lh->n;
//  for (i = 0; i < loop_log.lh.n; i++) {
//    loop_log.lh.block[i] = lh->block[i];
//  }
//  brelse(buf);
//}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
write_head_loop(void)
{
  if (!is_loop_mounted) {
    panic("[write_head_loop] loop dev not mounted");
  }
  struct buf *buf = bread(loop_log.dev, loop_log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = loop_log.lh.n;
  for (i = 0; i < loop_log.lh.n; i++) {
    hb->block[i] = loop_log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();
  install_trans(); // if committed, copy from log to disk
  log.lh.n = 0;
  loop_log.lh.n = 0;
  write_head(); // clear the log
}

// called at the start of each FS system call.
void
begin_op(void)
{
  // no need to use loop_lop.committing we always lock loop_log with log
  acquire(&log.lock);
  while(1){
    if(log.committing || (is_loop_mounted && loop_log.committing)) {
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE || (is_loop_mounted && loop_log.lh.n + (loop_log.outstanding+1)*MAXOPBLOCKS > LOGSIZE)) {
      // this op might exhaust log space; wait for commit.
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      loop_log.outstanding += 1; // should be the same as log.outstanding
      release(&log.lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  loop_log.outstanding -= 1; // should be the same as log.outstanding

  if(log.committing || loop_log.committing)
    panic("log is committing");
  if(log.outstanding == 0 && loop_log.outstanding == 0){
    do_commit = 1;
    log.committing = 1;
    loop_log.committing = 1;
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has decreased
    // the amount of reserved space.
    wakeup(&log);
  }
  release(&log.lock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    commit();
    acquire(&log.lock);
    log.committing = 0;
    loop_log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// Copy modified blocks from cache to log.
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // log block
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

// Copy modified blocks from cache to loop_log.
static void
write_log_loop(void)
{
  if (!is_loop_mounted) {
    panic("[write_log_loop] loop dev not mounted");
  }
  int tail;
  for (tail = 0; tail < loop_log.lh.n; tail++) {
    struct buf *to = bread(loop_log.dev, loop_log.start+tail+1); // log block
    struct buf *from = bread(loop_log.dev, loop_log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // write the log
    brelse(from);
    brelse(to);
  }
}

static void
commit()
{
  if (is_loop_mounted && loop_log.lh.n > 0) {
    write_log_loop();     // Write modified blocks from cache to log
    write_head_loop();    // Write header to disk -- the real commit
    install_trans_loop(); // Now install writes to home locations
    write_head_loop();    // Erase the transaction from the log
  }

  if (is_loop_mounted && loop_log.lh.n > 0 && log.lh.n == 0) {
    panic("[commit] loop_log is written but log is not written!");
  }

  if (log.lh.n > 0) {
    write_log();     // Write modified blocks from cache to log
    write_head();    // Write header to disk -- the real commit
    install_trans(); // Now install writes to home locations
    write_head();    // Erase the transaction from the log
  }

  log.lh.n = 0;
  loop_log.lh.n = 0;

}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache with B_DIRTY.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  if (!isloopdev(b->dev)) {
    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
      panic("too big a transaction");
    if (log.outstanding < 1)
      panic("log_write outside of trans");

    acquire(&log.lock);
    for (i = 0; i < log.lh.n; i++) {
      if (log.lh.block[i] == b->blockno)   // log absorbtion
        break;
    }
    log.lh.block[i] = b->blockno;
    if (i == log.lh.n)
      log.lh.n++;
    b->flags |= B_DIRTY; // prevent eviction
    release(&log.lock);
  } else {
    if (loop_log.lh.n >= LOGSIZE || loop_log.lh.n >= loop_log.size - 1)
      panic("too big a transaction");
    if (loop_log.outstanding < 1)
      panic("log_write outside of trans");

    acquire(&log.lock);
    for (i = 0; i < loop_log.lh.n; i++) {
      if (loop_log.lh.block[i] == b->blockno)   // log absorbtion
        break;
    }
    loop_log.lh.block[i] = b->blockno;
    if (i == loop_log.lh.n)
      loop_log.lh.n++;
    b->flags |= B_DIRTY; // prevent eviction
    release(&log.lock);
  }

}

