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
    initlock(&pid_ns_table.lock, "pid_ns_table");
    for(int i = 0; i < NNAMESPACE; ++i){
        initlock(&pid_ns_table.pid_namespaces[i].lock, "pid_ns_table");
        &pid_ns_table.pid_namespaces[i].count = 0;
    }
}
