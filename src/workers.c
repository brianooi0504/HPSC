#include "starpu.h"

int starpu_init(void) {
    struct starpu_task_list* task_list = malloc(sizeof(struct starpu_task_list));
    struct starpu_data_handle_list* data_handle_list = malloc(sizeof(struct starpu_data_handle_list));

    starpu_task_list_init(task_list);
    starpu_data_handle_list_init(data_handle_list);

    starpu_create_worker();
    starpu_create_worker();

    printf("StarPU initiailized\n");

    return 0;
}

void starpu_create_worker(void) {
    if (pipe(worker_pipe) == -1) {
        exit(-1);
    }

    pid_t pid = fork();

    if(pid == 0) {
        /* child process */
        printf("CHILD PROCESS %d: created\n", getpid());
        starpu_task_read_and_run();
        exit(0);
    }
}

void starpu_shutdown(void) {
    printf("StarPU shut down\n");
}