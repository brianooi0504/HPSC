#ifndef __STARPU_TASK_H__
#define __STARPU_TASK_H__

#include "starpu.h"
#include "starpu_enum.h"

#define TYPE float

#define STARPU_MAXIMPLEMENTATIONS 10
#define STARPU_NMAXBUFS 10
#define STARPU_NMAXDEPENDENCIES 10

#define STARPU_MAIN_RAM 0

typedef void (*starpu_cpu_func_t) (void **, void *);

struct starpu_codelet {
    starpu_cpu_func_t cpu_funcs[STARPU_MAXIMPLEMENTATIONS];
    const char *cpu_funcs_name[STARPU_MAXIMPLEMENTATIONS];
    int nbuffers; // number of args taken by the codelet
    enum starpu_data_access_mode modes[STARPU_NMAXBUFS]; //describes the required access modes to the data needed by the codelet
} ;

struct starpu_task {
    struct starpu_codelet* cl;
    struct starpu_data_handle* handles[STARPU_NMAXBUFS]; //struct data will be here 
    int version_req[STARPU_NMAXBUFS];
    void *cl_arg;
    size_t cl_arg_size;
    uint64_t tag_id;
    int priority;
    enum starpu_task_status status;
    struct starpu_task* next_task;
    struct starpu_task* self_id;

    int num_dependencies;
    struct starpu_task* dependencies[STARPU_NMAXDEPENDENCIES]; // tasks that must be completed before this one
    int num_dependencies_met;
};

struct starpu_task_list {
    pthread_mutex_t lock;
    struct starpu_task* head;
    struct starpu_task* tail;
};

struct starpu_func_arg {
    void* arg1;
    uint64_t tag_id;
};

struct starpu_task* starpu_task_create(
    struct starpu_codelet* cl,
    void* cl_arg,
    size_t cl_arg_size,
    uint64_t tag_id
);

void starpu_task_submit(struct starpu_task* task);
struct starpu_task* starpu_task_get(void);

void starpu_task_list_init(struct starpu_task_list *list);
void starpu_task_add_dependency(struct starpu_task* task, struct starpu_task* dependency);

void starpu_task_run(struct starpu_task* task, int notif_pipe_fd,starpu_task_spawn_mode mode);

void starpu_task_read_and_run(int worker_pipe_fd, int notif_pipe_fd, starpu_task_spawn_mode mode);
void starpu_task_wait_and_spawn(starpu_task_spawn_mode mode);
struct starpu_task* starpu_task_read(int worker_pipe_fd, starpu_task_spawn_mode mode);
void starpu_task_spawn(struct starpu_task* task, starpu_task_spawn_mode mode);

void* starpu_arg_init(void* arg1, uint64_t tag_id);

#endif /* __STARPU_TASK_H__ */