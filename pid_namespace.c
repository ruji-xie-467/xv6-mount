//
// Created by Zhihao Qin on 11/1/20.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pid_namespace.h"
#include "namespace.h"

struct {
    struct spinlock lock;
    struct pid_namespace pid_namespaces[NNAMESPACE];
} pid_ns_table;

void set_pid_namespace(struct pid_namespace* pid_namespace, int count, struct pid_namespace* parent_namespace, int next_pid, bool is_pid_1_killed){
    //set namespace count
    pid_namespace->count = count;
    //set parent namespace
    pid_namespace->parent = parent_namespace;
    //set next pid 
    pid_namespace->next_pid = next_pid;
    //set is pid 1 killed or not 
    pid_namespace->is_pid_1_killed = is_pid_1_killed;
}

void init_pid_namespaces(void) {
    //init lock for pid_namespace_table
    initlock(&pid_ns_table.lock, "pid_ns_table_lock");

    //init pid_namespace[i]
    int i = 0;
    while(i < NNAMESPACE){
        //init lock for pid_namespace
        initlock(&pid_ns_table.pid_namespaces[i].lock, "pid_ns_table_lock");
        //init pid_namespace features
        set_pid_namespace(&(pid_ns_table.pid_namespaces[i]), 0, NULL, 0, false);
        ++i;
    }
}

void remove_from_pid_namespace(struct pid_namespace* pid_namespace){
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);
    //reduce namespaces count by 1
    pid_namespace->count--;
    //get error if there are no pid_namespaces
    if(pid_namespace->count < 0){
        //reset pid_namespace->count
        pid_namespace->count++;
        panic("remove_from_pid_namespace: this pid_namespace is never used");
    }
    //release lock
    release(&pid_ns_table.lock);
}

struct pid_namespace* create_new_pid_namespace(struct pid_namespace* parent_namespace) {
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);

    int count = NNAMESPACE;
    struct pid_namespace* pid_ns = NULL;
    while(count > 0){
        if(pid_ns == NULL){
            pid_ns = &pid_ns_table.pid_namespaces[0];
        }
        if (pid_ns->count == 0) {
            set_pid_namespace(pid_ns, 1, parent_namespace, 1, false);
            release(&pid_ns_table.lock);
            return pid_ns;
        }else{
            pid_ns++;
            count--;
        }
    }
    //release lock
    release(&pid_ns_table.lock);
    //panic if all namespaces are occupied
    panic("all pid_namespaces are occupied");
}

struct pid_namespace* pid_ns_dup(struct pid_namespace* pid_ns) {
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);
    //increase namespaces count by 1
    pid_ns->count++;
    //release lock
    release(&pid_ns_table.lock);
    return pid_ns;
}

int pid_namespace_get_next_pid(struct pid_namespace* pid_ns) {
    //get lock first to reduce race condition
    acquire(&pid_ns->lock);
    int next_pid = pid_ns->next_pid;
    pid_ns->next_pid++;
    release(&pid_ns->lock);
    //release lock
    return next_pid;
}

int pid_ns_is_max_depth(struct pid_namespace* pid_ns){
    int depth = 0;
    while (pid_ns) {
        depth++;
        //check if larger than PID_NAMESPACE_MAX_DEPTH
        if(depth >= PID_NAMESPACE_MAX_DEPTH){
            return true;
        }

        //get parent
        pid_ns = pid_ns->parent;
    }
    return false;
}