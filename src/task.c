#include <string.h>
#include "starpu.h"

struct starpu_task* starpu_task_create(void) {
    struct starpu_task* task;

    task = malloc(sizeof(struct starpu_task));

    memset(task, 0, sizeof(struct starpu_task));

    task->next_task = NULL;

    return task;
}

void starpu_task_submit(struct starpu_task* task) {
    pthread_mutex_lock(&task_list.lock);

    if (task_list.tail) {
        task_list.tail->next_task = task;
    } else {
        task_list.head = task;
    }
    task_list.tail = task;

    pthread_mutex_unlock(&task_list.lock);
}

struct starpu_task* starpu_task_get(void) {
    struct starpu_task* next = NULL;

    pthread_mutex_lock(&task_list.lock);

    if (task_list.head) {
        next = task_list.head;

        task_list.head = next->next_task;

        if (!task_list.head) {
            task_list.tail = NULL;
        }
    }

    pthread_mutex_unlock(&task_list.lock);

   return next;
}

void starpu_task_list_init(struct starpu_task_list *list) {
    pthread_mutex_init(&list->lock, NULL);
    list->head = NULL;
    list->tail = NULL;
}

void starpu_task_wait_for_all(void) {
    struct starpu_task* cur;

    while (1) {
        pthread_mutex_lock(&task_list.lock);

        cur = starpu_task_get();

        pthread_mutex_unlock(&task_list.lock);

        if (cur) {
            starpu_task_run(cur);
        } else {
            break;
        }
    }

}

struct starpu_task* starpu_task_read(void) {
    struct starpu_task* t = malloc(sizeof(struct starpu_task));
    struct starpu_data_handle* h = malloc(sizeof(struct starpu_data_handle));
    float* user_data  = malloc(8*sizeof(float));
    float arg;
    size_t udata_size;

    read(worker_pipe[0], &t->version_req[0], sizeof(int));
    read(worker_pipe[0], &t->cl_arg_size, sizeof(size_t));
    read(worker_pipe[0], &arg, t->cl_arg_size);
    read(worker_pipe[0], &h->version_exec, sizeof(int));
    read(worker_pipe[0], &udata_size, sizeof(size_t));
    
    for (int i = 0; i < 8; i++) {
        read(worker_pipe[0], &user_data[i], sizeof(float));
    }

    h->user_data = user_data;
    t->cl_arg = &arg;
    t->handles[0] = h;

    return t;
}

void starpu_task_read_and_run(void) {
    struct starpu_task* cur;

    while (1) {
        cur = starpu_task_read();
        printf("CHILD PROCESS %d: read task\n", getpid());

        if (cur) {
            starpu_task_run(cur);
        }
    }
}

void starpu_task_spawn(struct starpu_task* task, enum starpu_task_spawn_mode mode) {
    if (mode == LOCAL_PROCESS) {
        printf("Task spawn\n");
        size_t udata_size = (task->handles[0]->nx)*(task->handles[0]->elem_size);

        // cl -> cpu_funcs
        write(worker_pipe[1], &task->version_req[0], sizeof(int));
        write(worker_pipe[1], &task->cl_arg_size, sizeof(size_t));
        write(worker_pipe[1], (float*) task->cl_arg, task->cl_arg_size);
        write(worker_pipe[1], &task->handles[0]->version_exec, sizeof(int));
        write(worker_pipe[1], &udata_size, sizeof(size_t));

        for (int i = 0; i < 8; i++) {
            write(worker_pipe[1], &task->handles[0]->user_data[i], sizeof(float));
        }
        
    }
}

void starpu_task_wait_and_spawn(void) {
    struct starpu_task* cur;

    while (1) {
        pthread_mutex_lock(&task_list.lock);

        cur = starpu_task_get();

        pthread_mutex_unlock(&task_list.lock);

        if (cur) {
            starpu_task_spawn(cur, LOCAL_PROCESS);
        }
    }

}

void starpu_task_run(struct starpu_task* task) {
    printf("Running task\n");
    // struct starpu_codelet* cl = task->cl;

    // starpu_cpu_func_t func = cl->cpu_funcs[0];

    int version_req = task->version_req[0];
    struct starpu_data_handle* handle = task->handles[0];

    //print version_req and exec_version
    printf("Version req: %d\n", version_req);
    printf("Handle exec version: %d\n", handle->version_exec);

    // Check data with version, while version_req is higher than exec, wait for the data to be ready
    while (handle->version_exec < version_req) {
        // Wait for the data to be ready
        printf("Waiting for data to be ready\n");
    }

    // func((void *) handle->user_data, task->cl_arg);

    handle->version_exec++;

    //print handle exec version
    printf("Handle exec version: %d\n", handle->version_exec);
}