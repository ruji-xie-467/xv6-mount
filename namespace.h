//
// Created by Tianle Zhang on 2020/10/22.
//
#include "types.h"
#include "spinlock.h"
#include "param.h"

#ifndef XV6_510_PROJECT_NAMESPACE_H
#define XV6_510_PROJECT_NAMESPACE_H

typedef struct nsproxy nsproxy_struct;

struct nsproxy {
    int count;
    struct pid_namespace *pid_ns;
};

struct {
    struct spinlock lock;
    nsproxy_struct nsproxy[NNAMESPACE];
} nstable;


#endif //XV6_510_PROJECT_NAMESPACE_H
