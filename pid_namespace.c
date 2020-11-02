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
#include "wstatus.h"
#include "pid_namespace.h"
#include "namespace.h"

struct {
    struct spinlock lock;
    struct pid_ns pid_namespaces[NNAMESPACE];
} pid_ns_table;

void pid_ns_init() {
    //init lock for pid_namespace_table
    initlock(&pid_ns_table.lock, "pid_ns_table");
    for(int i = 0; i < NNAMESPACE; ++i){
        //init lock for each pid_namespace
        initlock(&pid_ns_table.pid_namespaces[i].lock, "pid_ns_table");
        //set pid_namespace_count as 0
        &pid_ns_table.pid_namespaces[i].count = 0;
    }
}

void pid_ns_put(struct pid_ns* pid_namespace)
{
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);
    //get error if there are no pid_namespaces
    if(pid_namespace->count == 0){
        panic("pid_ns_put: count is 0");
    }
    //reduce namespaces count by 1
    pid_namespace->count--;
    //release lock
    release(&pid_ns_table.lock);
}

void pid_ns_get(struct pid_ns* pid_ns){
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);
    //increase namespaces count by 1
    pid_namespace->count++;
    //release lock
    release(&pid_ns_table.lock);
}

struct pid_ns* pid_ns_alloc() {
    //get lock first to reduce race condition
    acquire(&pid_ns_table.lock);
    for (int i = 0; i < NNAMESPACE; i++) {
        //iterate until we found an empty entry in pid_ns_table
        struct pid_ns* pid_ns = &pid_ns_table.pid_namespaces[i];
        //return if this is an empty entry
        if (pid_ns->count == 0) {
            pid_ns->count = 1;
            release(&pid_ns_table.lock);
            return pid_ns;
        }
    }
    //panic if all namespaces are occupied
    release(&pid_ns_table.lock);
    panic("all pid_ns objects are occupied");
}

void pid_ns_init_ns(struct pid_ns* pid_ns, struct pid_ns* parent_ns) {
    //set parent namespace
    pid_ns->parent = parent_ns;
    //set next pid 1
    pid_ns->next_pid = 1;
    //set pid 1 is not killed
    pid_ns->is_pid_1_killed = 0;
}

struct pid_ns* pid_ns_dup(struct pid_ns* pid_ns) {
    //get target namespace
    pid_ns_get(pid_ns);
    return pid_ns;
}

struct pid_ns* pid_ns_new(struct pid_ns* parent_namespace) {
    //alloc a new namespace first
    struct pid_ns * pid_ns = pid_ns_alloc();
    //init namespace
    pid_ns_init_ns(pid_ns, parent_namespace);
    return pid_ns;
}

int pid_ns_next_pid(struct pid_ns* pid_ns) {
    //get lock first to reduce race condition
    acquire(&pid_ns.lock);
    int pid = pid_ns->next_pid++;
    release(&pid_ns->lock);
    //release lock
    return pid;
}

int pid_ns_is_max_depth(struct pid_ns* pid_ns){
    int depth = 0;
    while (pid_ns) {
    depth++;
    pid_ns = pid_ns->parent;
    }
    return depth >= MAX_PID_NS_DEPTH;
}