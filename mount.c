#include "types.h"
#include "stat.h"
#include "user.h"
 
int main(int argc, char *argv[]) {
  if(argc < 3){
    printf(2, "Usage: mount device mountpoint\n");
    exit(0);
  }

  if (mount(argv[1], argv[2])) {
    printf(2, "Error: couldn't mount %s to %s\n", argv[2], argv[1]);
  }
  exit(0);
}
