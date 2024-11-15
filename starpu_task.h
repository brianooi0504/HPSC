#ifndef __STARPU_TASK_H__
#define __STARPU_TASK_H__

#include <starpu.h>
#include <errno.h>
#include <assert.h>

#define STARPU_MAXIMPLEMENTATIONS 100
#define STARPU_NMAXBUFS 100

#define STARPU_MAIN_RAM 0

typedef void (*starpu_cpu_func_t) (void **, void *);
typedef uint64_t starpu_tag_t;

// typedef struct {
//     int type;
//     int value;
// } task_param;

// typedef struct {
//     int num_params;
//     task_param* params;
// } task_data;

// typedef struct {
//     void* f;
//     int status;
//     task_data* data;
// } task;

enum starpu_task_status {
    TASK_INIT, // initialized
    TASK_READY, // ready for execution
    TASK_RUNNING, // running on some worker
    TASK_FINISHED, // finished executing
};

typedef struct {

} starpu_data_handler;

struct starpu_codelet {
    starpu_cpu_func_t cpu_funcs[STARPU_MAXIMPLEMENTATIONS];
    const char *cpu_funcs_name[STARPU_MAXIMPLEMENTATIONS];
    int nbuffers; // number of args taken by the codelet
    enum starpu_data_access_mode modes[STARPU_NMAXBUFS]; //describes the required access modes to the data needed by the codelet
} ;

struct starpu_data {
    void* ptr;
    size_t size;
    int version;
    int exec_version;
};

struct starpu_task{
    struct starpu_codelet* cl;
    starpu_data_handle_t handles[STARPU_NMAXBUFS]; //struct data will be here 
    void *cl_arg;
    size_t cl_arg_size;
    starpu_tag_t tag_id;
    int priority;
    enum starpu_task_status status;
};

void starpu_codelet_init() {

}

struct starpu_task* starpu_task_create(void);
int starpu_task_submit(struct starpu_task* task);

#endif /* __STARPU_TASK_H__ */