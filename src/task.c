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

    read(worker_pipe[0], t, sizeof(struct starpu_task));
    printf("%d read task %p\n", getpid(), t);

    for (int i = 0; i < t->cl->nbuffers; i++) {
        printf("Read data handle %p\n", t->data_pointers[i]);
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
     
    }
}

void starpu_task_wait_and_spawn(void) {
    struct starpu_task* cur = NULL;

    while (task_completion_counter < task_spawn_counter) {
        pthread_mutex_lock(&task_list.lock);

        cur = starpu_task_get();

        pthread_mutex_unlock(&task_list.lock);

        if (cur) {
            starpu_task_spawn(cur, LOCAL_PROCESS);
            cur = NULL;
        }
    }

}

void* starpu_arg_init(void* arg1, void* tag_id) {
    struct starpu_func_arg* args = (struct starpu_func_arg*) malloc(sizeof(struct starpu_func_arg));

    if (arg1) {
        args->arg1 = arg1;
    }
    if (tag_id) {
        args->tag_id = tag_id;
    }

    return (void *) args;
}

void starpu_task_run(struct starpu_task* task) {
    printf("Running task\n");
    task->status = TASK_RUNNING;
    struct starpu_codelet* cl = task->cl;

    starpu_cpu_func_t func = cl->cpu_funcs[0];

    void* func_arg = starpu_arg_init(task->cl_arg, task->tag_id);

    func((void *) task->data_pointers, func_arg);

    write(notification_pipe[1], &task->self_id, sizeof(struct starpu_task*));
}