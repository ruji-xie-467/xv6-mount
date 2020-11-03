//
// Created by Tianle Zhang on 2020/11/3.
//
#include "syscall.h"
#include "types.h"
#include "user.h"
#include "test_utility.h"

#define PID_NS 0b00000001

#define ASSERT(x, msg) \
do { \
  if (!(x)) { \
    printf(1, "assert fails: ");\
    printf(1, msg);\
    printf(1, "\n");\
  } \
} while (0)

int assert_non_negtive(int r, char *msg) {
  if (r < 0) {
      printf(1, "assert fails: ");
      printf(1, msg);
      printf(1, "\n");
  }
  return r;
}

/* Verify simple pid namespace works
  - Unshare pid ns
  - Fork
  - Verify pid is correct from parent and child views
*/
int test_simple_pidns() {
    ASSERT(unshare(PID_NS), "failed to unshare");
    int ret = assert_non_negtive(fork(), "failed to fork");
    // child
    if (ret == 0) {
        ASSERT(getpid() == 1, "pid not equal to 1");
        exit();
    }

    // flaky test because pid can recycle. However strictly speaking pid should be
    // increasing
    ASSERT(getpid() < ret, "wrong pid");

//    int status = child_exit_status(ret);
//    ASSERT(status == 0, "child process failed");
    return 0;
}

int main() {
//    run_test(test_simple_pidns, "test_simple_pidns");
    test_simple_pidns();
}