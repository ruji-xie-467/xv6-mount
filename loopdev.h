#define NLOOPDEV 20
#define LOOPDEV_MASK 0b100000 // NLOOPDEV is 20, so this mask is 32
#define LOOPDEV_MASKBIT 5

struct loopdev {
  int used;
  struct inode * ip;
};