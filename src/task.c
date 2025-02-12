#include "starpu.h"

struct starpu_task* starpu_task_create(
    struct starpu_codelet* cl,
    void* cl_arg,
    size_t cl_arg_size,
    uint64_t tag_id
) {
    struct starpu_task* task;

    task = malloc(sizeof(struct starpu_task));

    memset(task, 0, sizeof(struct starpu_task));

    task->cl = cl;
    task->cl_arg = cl_arg;
    task->cl_arg_size = cl_arg_size;
    task->tag_id = tag_id;
    task->next_task = NULL;
    task->status = TASK_INIT;

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
    task->status = TASK_READY;

    pthread_mutex_unlock(&task_list.lock);
}

struct starpu_task* starpu_task_get(void) {
    struct starpu_task* task = NULL;

    pthread_mutex_lock(&task_list.lock);

    task = task_list.head;

    while (task) {
        if (task->status == TASK_READY) {
            int all_versions_match = 1;
            for (int i = 0; i < task->cl->nbuffers; i++) {
                if (task->handles[i]->version_exec < task->version_req[i]) {
                    all_versions_match = 0;
                    break;
                }
            }
            if (all_versions_match) {
                task->status = TASK_ASSIGNED;
                pthread_mutex_unlock(&task_list.lock);
                return task;
            }
        }
        task = task->next_task;
    }

    pthread_mutex_unlock(&task_list.lock);

    return NULL;
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

    read(worker_pipe[0], t, sizeof(struct starpu_task));

    for (int i = 0; i < t->cl->nbuffers; i++) {
        read(worker_pipe[0], h, sizeof(struct starpu_data_handle));

        t->handles[i] = h;
    }

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

        task->self_id = task;

        write(worker_pipe[1], task, sizeof(struct starpu_task));

        for (int i = 0; i < task->cl->nbuffers; i++) {
            write(worker_pipe[1], task->handles[i], sizeof(struct starpu_data_handle));
        }
     
    }
}

void starpu_task_wait_and_spawn(void) {
    struct starpu_task* cur;

    while (task_completion_counter < task_spawn_counter) {
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
    task->status = TASK_RUNNING;
    struct starpu_codelet* cl = task->cl;

    starpu_cpu_func_t func = cl->cpu_funcs[0];

    struct starpu_data_handle* handle = task->handles[0];

    func((void *) handle->user_data, task->cl_arg);

    write(notification_pipe[1], &task->self_id, sizeof(struct starpu_task*));
}