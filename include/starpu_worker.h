#ifndef __STARPU_WORKER_H__
#define __STARPU_WORKER_H__

#include "starpu.h"
#include "starpu_enum.h"

struct starpu_worker {
    pid_t pid;
    int worker_pipe[2];
    int notif_pipe[2];
};

struct notification_listener_args {
    enum starpu_task_spawn_mode mode;
    int notif_pipe_fd;
};

extern struct starpu_worker* workers;
extern int num_workers;

#endif /* __STARPU_WORKER_H__ */