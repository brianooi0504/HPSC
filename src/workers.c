#include "starpu.h"

int starpu_init(void) {
    struct starpu_task_list* task_list = malloc(sizeof(struct starpu_task_list));
    struct starpu_data_handle_list* data_handle_list = malloc(sizeof(struct starpu_data_handle_list));

    starpu_task_list_init(task_list);
    starpu_data_handle_list_init(data_handle_list);

    if (pipe(worker_pipe) == -1) {
        exit(-1);
    }

    starpu_create_worker();
    // starpu_create_worker();

    close(worker_pipe[0]);
    printf("StarPU initiailized\n");

    return 0;
}

void starpu_create_worker(void) {
    pid_t pid = fork();

    if(pid == 0) {
        /* child process */
        printf("CHILD PROCESS %d: created\n", getpid());
        starpu_task_read_and_run();
        close(worker_pipe[1]);
    }
}

void starpu_shutdown(void) {
    printf("StarPU shut down\n");
}