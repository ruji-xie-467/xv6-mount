#include "types.h"
#include "stat.h"
#include "user.h"
 
int main(int argc, char *argv[]) {
  if(argc < 2){
    printf(2, "Usage: umount mountpoint\n");
    exit(0);
  }

  if (umount(argv[1])) {
    printf(2, "Error: couldn't umount %s\n", argv[1]);
  }
  exit(0);
}
