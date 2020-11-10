//
// Created by Tianle Zhang on 2020/10/22.
//

#include "namespace.h"
#include "defs.h"
#include "memlayout.h"
#include "x86.h"
#include "mmu.h"
#include "proc.h"

void ns_init() {
    initlock(&nstable.lock, "nstable");
    //init pid_namepsace
    init_pid_namespaces();
}

/*
 * The nsproxy count member is a reference counter, which is initialized to 1 when the nsproxy object is created by the
 * create_nsproxy() method, and which is decremented by the put_nsproxy() method and incremented by the get_nsproxy() method.
 */
nsproxy_struct* create_nsproxy(struct pid_namespace * pid_namespace, bool is_lock_required) {
    if(is_lock_required){
        acquire(&nstable.lock);
    }

    for (int i = 0; i < NNAMESPACE; ++i) {
        //find an empty entry fom nstable
        if (nstable.nsproxy[i].count == 0) {
            nstable.nsproxy[i].count = 1;

            //init pid_namespace
            if(pid_namespace == NULL){
                nstable.nsproxy[i].pid_ns = create_new_pid_namespace(NULL);
            }else{
                nstable.nsproxy[i].pid_ns = increase_pid_namespace_count(pid_namespace);
            }
            
            if(is_lock_required){
                release(&nstable.lock);
            }

            return &(nstable.nsproxy[i]);
        }
    }
    panic("namespace out of max!\n");
}

void free_nsproxy(nsproxy_struct* nsproxy) {
    acquire(&nstable.lock);
    //TODO: pid & other
    release(&nstable.lock);
}

void put_nsproxy(nsproxy_struct* nsproxy) {
    acquire(&nstable.lock);
    nsproxy->count--;
    if (nsproxy->count == 0) {
        remove_from_pid_namespace(nsproxy->pid_ns);
        nsproxy->pid_ns = NULL;
    }
    release(&nstable.lock);
}

void get_nsproxy(nsproxy_struct* nsproxy) {
    acquire(&nstable.lock);
    nsproxy->count++;
    release(&nstable.lock);
}

nsproxy_struct* increase_nsproxy_count(nsproxy_struct* nsproxy){
    get_nsproxy(nsproxy);
    return nsproxy;
}

/*
 * unshare allows a process to 'unshare' part of the process
 * context which was originally shared using clone. This system call gets
 * a single parameter that is a bitmask of CLONE* flags.
 */
int unshare(int flags) {
    acquire(&nstable.lock);
    struct proc* p = myproc();
    nsproxy_struct* old_ns = p->nsproxy;
    if (!(old_ns->count > 1)) {
        panic("assert fails. namespace.c: 75\n"); // is there any situation that count <= 1 when unshare? (not sure)
    }
    p->nsproxy = create_nsproxy(old_ns->pid_ns, false);  // should have a different ns now
    release(&nstable.lock);

    put_nsproxy(old_ns);

    if ((flags & PID_NS) > 0) {
        //check if already unshared
        if(p->child_pid_namespace){
            panic("pid_namespace already unshared");
            return -1;
        }

        //check if enough depth available
        if (!check_is_pid_namespace_legal(p->nsproxy->pid_ns)) {
            panic("no depth available for new child pid_namespace");
            return -1;
        }

        //create new child namespace
        p->child_pid_namespace = create_new_pid_namespace(p->nsproxy->pid_ns);
    }
    return 0;
}