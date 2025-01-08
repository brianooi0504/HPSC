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

void starpu_task_run(struct starpu_task* task) {
    struct starpu_codelet* cl = task->cl;

    starpu_cpu_func_t func = cl->cpu_funcs[0];

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

    func((void *) handle->user_data, task->cl_arg);

    handle->version_exec++;

    //print handle exec version
    printf("Handle exec version: %d\n", handle->version_exec);
}