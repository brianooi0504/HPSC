#include <starpu.h>

struct starpu_task* starpu_task_create(void) {
    struct starpu_task* task;

    task = malloc(sizeof(struct starpu_task));

    memset(task, 0, sizeof(struct starpu_task));

    return task;    
}

int starpu_task_submit(struct starpu_task* task) {
    int ret;
    
    return ret;
}