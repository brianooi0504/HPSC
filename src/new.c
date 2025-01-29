#include "starpu.h"

starpu_worker workers[4];
int worker_pipe[2];
int notification_pipe[2];

struct starpu_task read_task_from_master() {
    struct starpu_task* t = malloc(sizeof(struct starpu_task));
    read(worker_pipe[0], t->function);
    read(worker_pipe[0], t->data[0]);
    read(worker_pipe[0], t->data[1]);
    return t;
}

void starpu_wait_for_incoming_tasks() {
    while(1) {
        struct starpu_task t = read_task_from_master();

        run_task(t);
        notify_master_process(t);
    }
}

void spawn_remote_task(struct starpu_task t) {
    write(worker_pipe[1], t->function);
    write(worker_pipe[1], t->data);
}

void starpu_wait_all_tasks() {
    while(1) {
        get_notifications();
        struct starpu_task t = get_task();
        //    run_task(t);
        spawn_remote_task(t); // -> write to the pipe
    }
}

void create_worker() {
    worker_pipe(pipe_fd);

    pid_t pid = fork();
    if(pid == 0) {
        /* child process */
        starpu_wait_for_incoming_tasks();
        exit(0);
    }
}

void starpu_init() {
    create_worker();
    create_worker();
    create_worker();
    create_worker();
}

int main(){
    starpu_init();

    return 0;
}

