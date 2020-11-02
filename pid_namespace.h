//
// Created by Zhihao Qin on 11/1/20.
//

#ifndef XV6_510_PROJECT_PID_NAMESPACE_H
#define XV6_510_PROJECT_PID_NAMESPACE_H

#endif //XV6_510_PROJECT_PID_NAMESPACE_H

*define PID_NAMESPACE_MAX_DEPTH 4

struct pid_ns {
    int count;
    struct pid_ns* parent;
    struct spinlock lock;
    int next_pid;
    bool is_pid_1_killed;
};

void pid_ns_init();
void pid_ns_put(struct pid_ns* pid_ns);
void pid_ns_get(struct pid_ns* pid_ns);
int pid_ns_next_pid(struct pid_ns* pid_ns);
struct pid_ns* pid_ns_new(struct pid_ns* parent_ns);
struct pid_ns* pid_ns_dup(struct pid_ns* pid_ns);
int pid_ns_is_max_depth(struct pid_ns* pid_ns);
