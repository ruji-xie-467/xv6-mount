//
// Created by Tianle Zhang on 2020/10/22.
//
#include "types.h"
#include "spinlock.h"
#include "param.h"

#ifndef XV6_510_PROJECT_NAMESPACE_H
#define XV6_510_PROJECT_NAMESPACE_H

#endif //XV6_510_PROJECT_NAMESPACE_H

typedef struct nsproxy nsproxy_struct;

struct nsproxy {
    int count;
    struct pid_namespace *pid_ns;
//    struct uts_namespace *uts_ns;
//    struct ipc_namespace *ipc_ns;
//    struct mnt_namespace *mnt_ns;
//    struct net *net_ns;
};

struct {
    struct spinlock lock;
    struct nsproxy nsproxy[NNAMESPACE];
} nstable;



