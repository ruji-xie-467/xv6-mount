//
// Created by Zhihao Qin on 11/1/20.
//

#ifndef XV6_510_PROJECT_PID_NAMESPACE_H
#define XV6_510_PROJECT_PID_NAMESPACE_H



struct pid_namespace {
    int count;
    struct pid_namespace* parent;
    struct spinlock lock;
    int next_pid;
    int depth;
    bool is_pid_1_killed;
};

struct {
    struct spinlock lock;
    pid_namespace_struct pid_namespaces[NNAMESPACE];
} pid_ns_table;

#endif //XV6_510_PROJECT_PID_NAMESPACE_H