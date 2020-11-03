//
// Created by Zhihao Qin on 11/1/20.
//

#ifndef XV6_510_PROJECT_PID_NAMESPACE_H
#define XV6_510_PROJECT_PID_NAMESPACE_H

#endif //XV6_510_PROJECT_PID_NAMESPACE_H

#define PID_NAMESPACE_MAX_DEPTH 4

struct pid_namespace {
    int count;
    struct pid_namespace* parent;
    struct spinlock lock;
    int next_pid;
    bool is_pid_1_killed;
};
