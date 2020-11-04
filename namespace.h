//
// Created by Tianle Zhang on 2020/10/22.
//

#ifndef XV6_510_PROJECT_NAMESPACE_H
#define XV6_510_PROJECT_NAMESPACE_H

struct nsproxy {
    int count;
    struct pid_namespace *pid_ns;
//    struct uts_namespace *uts_ns;
//    struct ipc_namespace *ipc_ns;
//    struct mnt_namespace *mnt_ns;
//    struct net *net_ns;
};
struct nsproxy* namespace_replace_pid_ns(struct nsproxy* oldns, struct pid_namespace* pid_ns);


#endif //XV6_510_PROJECT_NAMESPACE_H
