//
// Created by Tianle Zhang on 2020/10/22.
//

#include "namespace.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define PID_NS 0x00000001

struct {
    struct spinlock lock;
    struct nsproxy nsproxy[NNAMESPACE];
} nstable;

void ns_init() {
    initlock(&nstable.lock, "nstable");
}

/*
 * The nsproxy count member is a reference counter, which is initialized to 1 when the nsproxy object is created by the
 * create_nsproxy() method, and which is decremented by the put_nsproxy() method and incremented by the get_nsproxy() method.
 */
struct nsproxy* create_nsproxy() {
    acquire(&nstable.lock);
    for (int i = 0; i < NNAMESPACE; ++i) {
        if (nstable.nsproxy[i].count == 0) {
            nstable.nsproxy[i].count = 1;
            //TODO: pid & other
            release(&nstable.lock);
            return &(nstable.nsproxy[i]);
        }
    }
    panic("namespace out of max!\n");
}

void free_nsproxy(struct nsproxy* nsproxy) {
    acquire(&nstable.lock);
    //TODO: pid & other
    release(&nstable.lock);
}

void put_nsproxy(struct nsproxy* nsproxy) {
    acquire(&nstable.lock);
    nsproxy->count--;
    if (nsproxy->count == 0) {
        free_nsproxy(nsproxy);
    }
    release(&nstable.lock);
}

void get_nsproxy(struct nsproxy* nsproxy) {
    acquire(&nstable.lock);
    nsproxy->count++;
    release(&nstable.lock);
}

/*
 * unshare allows a process to 'unshare' part of the process
 * context which was originally shared using clone. This system call gets
 * a single parameter that is a bitmask of CLONE* flags.
 */
int unshare(int flags) {
    acquire(&nstable.lock);
    struct proc* p = myproc();
    struct nsproxy* old_ns = p->nsproxy;
    if (!(old_ns->count > 1)) {
        panic("assert fails. namespace.c: 75\n"); // is there any situation that count <= 1 when unshare? (not sure)
    }
    p->nsproxy = create_nsproxy();  // should have a different ns now
    release(&nstable.lock);

    put_nsproxy(old_ns);

    if ((flags & PID_NS) > 0) {
        //TODO: pid
    }
    return 0;
}