#ifndef __STARPU_ENUM_H__
#define __STARPU_ENUM_H__

typedef enum starpu_task_spawn_mode {
    LOCAL,
    LOCAL_PROCESS,
    REMOTE_PROCESS,
} starpu_task_spawn_mode;

enum starpu_task_status {
    TASK_INIT, // initialized
    TASK_WAITING, // waiting for dependencies
    TASK_READY, // ready for execution
    TASK_ASSIGNED, // assigned to a worker
    TASK_RUNNING, // running on some worker
    TASK_FINISHED, // finished executing
};

enum starpu_data_access_mode {
    STARPU_R = (1 << 0), // read-only
    STARPU_W = (1 << 1), // write-only
    STARPU_RW = (STARPU_R | STARPU_W) // read-write
};

#endif