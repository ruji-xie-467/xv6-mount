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
    printf(2, "assert fails: ");\
    printf(2, msg);\
    printf(2, "\n");\
  } \
} while (0)

int assert_non_negtive(int r, char *msg) {
  if (r < 0) {
      printf(2, "assert fails: ");
      printf(2, msg);
      printf(2, "\n");
      exit(1);
  }
  return r;
}

int assert_true(int r, char *msg) {
  if (r == 0) {
      printf(2, "assert fails: ");
      printf(2, msg);
      printf(2, "\n");
      exit(1);
  }
  return r;
}

//Verify that after unshare, the first child process should have pid 1 in new pid_namespace
int test_proc_in_new_pid_namesapce_with_pid_1() {
  printf(1, "------------test1------------\n");
  int unshare_result = unshare(PID_NS);
  assert_non_negtive(unshare_result, "failed to unshare\n");

  int ret = assert_non_negtive(fork(), "failed to fork\n");
  if (ret == 0) {// child
    int test_pid = getpid();
    printf(1, "child pid:%d\n", test_pid);
    assert_true(test_pid == 1, "pid not equal to 1");
    exit(0);
  }else{// parent
    //wait for child process to finish
    wait();
    int test_pid = getpid();
    printf(1, "parent pid:%d\n", test_pid);
    printf(1, "test1 pass\n");
    printf(1, "------------------------------\n");
    return 0;
  }
}

//Verify that the process with PID 1 reaps the orphaned children
int test_reap_orphaned_children(){
  printf(1, "------------test2------------\n");

  int unshare_result = unshare(PID_NS);
  assert_non_negtive(unshare_result, "failed to unshare\n");

  //create child 1
  int ret = assert_non_negtive(fork(), "failed to fork\n");
  if (ret == 0) {// child1
    //child2's children
    int child_num = 2;
    signed int child_pids[2];
    int fd[2];

    assert_non_negtive(pipe(fd), "failed to fork\n");

    //create child 2
    int ret2 = assert_non_negtive(fork(), "failed to fork\n");
    if(ret2 == 0){
      //create child3 and child4
      for(int i = 0; i < child_num; ++i){
        int ret = assert_non_negtive(fork(), "failed to fork\n");
        if(ret == 0){
          //leave enough time to let child2 exit() first, so that child1 can reap child3 and child4
          sleep(100);
          exit(0);
        }else{
          //record child pids in child2
          child_pids[i] = ret;
        }
      }

      //write all child pids into child1 process
      signed int bytes_read = 0;
      signed int len = sizeof(child_pids);
      unsigned char *buf = (void *) child_pids;
      while (bytes_read < len) {
        signed int ret = assert_non_negtive(write(fd[1], &buf[bytes_read], len - bytes_read), "failed to write");
        bytes_read += ret;
      }

      //close fd
      close(fd[0]);
      close(fd[1]);

      //child2 exit
      exit(0);
    }else{
      //wait for child2 to exit
      wait();
      //read child3 and child4 pids into child_pid
      signed int bytes_read = 0;
      signed int len = sizeof(child_pids);
      unsigned char *buf = (void *) child_pids;
      while (bytes_read < len) {
        signed int ret = assert_non_negtive(read(fd[0], &buf[bytes_read], len - bytes_read), "failed to read");
        bytes_read += ret;
      }

      //wait for child3 and child4 exit()
      int count = 0;
      for(int i = 0; i < child_num; ++i){
        int child_pid = wait();
        for(int j = 0; j < child_num; ++j){
          if(child_pids[j] == child_pid){
            count++;
            printf(1, "child_pid %d is reaped by child_proc with pid %d\n", child_pid, getpid());
          }
        }
      }

      //assert all orphaned children are reaped
      assert_true(count == 2, "not all orphaned children are reaped");

      //close fd
      close(fd[0]);
      close(fd[1]);

      //child1 exit
      exit(0);
    }
  }else{// parent
    //wait for child1 to exit
    wait();
    printf(1, "test2 pass\n");
    printf(1, "------------------------------\n");
    return 0;
  }
}

//Verify that all children process should be killed when process with pid1 is killed
int test_all_proc_killed_after_pid1_killed(){
  printf(1, "------------test3------------\n");

  int unshare_result = unshare(PID_NS);
  assert_non_negtive(unshare_result, "failed to unshare\n");

  //create child 1
  int ret = assert_non_negtive(fork(), "failed to fork\n");
  if (ret == 0) {// child1
    //amount of child2's children
    int child_num = 2;

    //create child 2 in new pid_namespace
    int unshare_result = unshare(PID_NS);
    assert_non_negtive(unshare_result, "failed to unshare\n");
    int ret2 = assert_non_negtive(fork(), "failed to fork\n");
    if(ret2 == 0){//child2
      //assert child2's pid equal to 1
      assert_true(getpid() == 1, "child2 pid not equal to 1");

      //create child3 and child4
      for(int i = 0; i < child_num; ++i){
        int ret = assert_non_negtive(fork(), "failed to fork\n");
        if(ret == 0){
          //normally child3 and child4 shuold never exit, unless child2 exits, then child3 and child4 should auto exit
          while(1){}
          exit(0);
        }
      }

      //child2 exit
      exit(0);
    }else{
      //wait for child2 to exit
      int child2_pid = wait();

      //wait for child3 and child4 exit()
      int count = 0;
      for(int i = 0; i < child_num; ++i){
        int child_pid = wait();
        printf(1, "child%d is auto killed after child%d(with pid 1) is killed\n", child_pid, child2_pid);
        count++;
      }
      //assert all orphaned children are killed
      assert_true(count == 2, "not all orphaned children are killed");

      //child1 exit
      exit(0);
    }
  }else{// parent
    //wait for child1 to exit
    wait();
    printf(1, "test3 pass\n");
    printf(1, "------------------------------\n");
    return 0;
  }
}

int main() {
  int ret = -1;

  //run test1
  ret = fork();
  if(ret == 0){
    test_proc_in_new_pid_namesapce_with_pid_1();
    exit(0);
  }
  wait();

  //run test2
  ret = fork();
  if(ret == 0){
    test_reap_orphaned_children();
    exit(0);
  }
  wait();

  //run test3
  ret = fork();
  if(ret == 0){
    test_all_proc_killed_after_pid1_killed();
    exit(0);
  }
  wait();

  exit(0);
}