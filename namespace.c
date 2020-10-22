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

