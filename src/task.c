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
    func((void *) task->handles[0]->user_data, task->cl_arg);
}