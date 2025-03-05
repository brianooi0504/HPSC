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
    task->num_dependencies = 0;
    task->num_dependencies_met = 0;

    return task;
}

void starpu_task_submit(struct starpu_task* task) {
    for (int i = 0; i < task->cl->nbuffers; i++) {
        task->version_req[i] = task->handles[i]->version_req + 1;
        task->handles[i]->version_req++;
    }

    pthread_mutex_lock(&task_list->lock);

    // Check if all dependencies are met
    int all_dependencies_met = 1;
    for (int i = 0; i < task->num_dependencies; i++) {
        if (task->dependencies[i]->status != TASK_FINISHED) {
            all_dependencies_met = 0;
            pthread_mutex_unlock(&task_list->lock);
            break;
        }
    }

    // If dependencies are met, mark as TASK_READY, otherwise TASK_WAITING
    if (all_dependencies_met) {
        task->status = TASK_READY;
        printf("Task %p submitted READY\n", task);
    } else {
        task->status = TASK_WAITING;
        printf("Task %p submitted WAITING\n", task);
    }

    // Add task to the queue
    if (task_list->tail) {
        task_list->tail->next_task = task;
    } else {
        task_list->head = task;
    }
    task_list->tail = task;

    pthread_mutex_unlock(&task_list->lock);
}

struct starpu_task* starpu_task_get(void) {
    struct starpu_task* task = NULL;

    task = task_list->head;

    while (task) {
        if (task->status == TASK_READY) {
            int all_versions_match = 1;

            // Check data versions
            for (int i = 0; i < task->cl->nbuffers; i++) {
                if (task->handles[i]->version_exec < task->version_req[i]) {
                    all_versions_match = 0;
                    break;
                }
            }

            if (task->num_dependencies_met < task->num_dependencies) {
                all_versions_match = 0;
            }

            if (all_versions_match) {
                task->status = TASK_ASSIGNED;
                return task;
            }
        }
        task = task->next_task;
    }

    return NULL;
}

void starpu_task_list_init(struct starpu_task_list *list) {
    pthread_mutex_init(&list->lock, NULL);
    list->head = NULL;
    list->tail = NULL;
}

void starpu_task_add_dependency(struct starpu_task* task, struct starpu_task* dependency) {
    printf("Task %p with dependency %p\n", task, dependency);
    if (task->num_dependencies < STARPU_NMAXDEPENDENCIES) {
        task->dependencies[task->num_dependencies] = dependency;
        task->num_dependencies++;
    } else {
        printf("Error: Too many dependencies for task %p\n", task);
    }
}

struct starpu_task* starpu_task_read(int worker_pipe_fd, starpu_task_spawn_mode mode) {
    // Allocate memory for the task
    struct starpu_task* task = malloc(sizeof(struct starpu_task));

    if (!task) {
        perror("Failed to allocate memory for task");
        return NULL;
    }

    // Read the task structure from the pipe
    read(worker_pipe_fd, task, sizeof(struct starpu_task));

    // For REMOTE_PROCESS mode, deserialize the user_data for each handle
    if (mode == REMOTE_PROCESS) {
        for (int i = 0; i < task->cl->nbuffers; i++) {
            struct starpu_data_handle* handle = task->handles[i];

            // Read the size of the user_data
            size_t data_size;

            if (read_exact(worker_pipe_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
                perror("Failed to read full data_size");
                free(task);
                return NULL;
            }

            // Allocate memory for the user_data
            handle->user_data = malloc(data_size);
            handle->elem_size = sizeof(TYPE);
            handle->nx = data_size / handle->elem_size;
            if (!handle->user_data) {
                perror("Failed to allocate memory for user_data");
                free(task);
                return NULL;
            }

            // Read the serialized data into user_data
            if (read_exact(worker_pipe_fd, handle->user_data, data_size) != data_size) {
                perror("Failed to read full user_data");
                free(task);
                return NULL;
            }

            printf("CHILD PROCESS %d: Deserialized data handle %p (size: %zu)\n", getpid(), handle, data_size);
        }
    }

    printf("CHILD PROCESS %d: Task %p read successfully\n", getpid(), task);
    return task;
}

void starpu_task_read_and_run(int worker_pipe_fd, int notif_pipe_fd, starpu_task_spawn_mode mode) {
    struct starpu_task* cur;

    while (1) {
        cur = starpu_task_read(worker_pipe_fd, mode);

        if (cur) {
            printf("CHILD PROCESS %d: Task read\n", getpid());
            starpu_task_run(cur, notif_pipe_fd, mode);
        }
    }
}

void* task_spawning_worker(void* arg) {
    struct spawner_args* args = (struct spawner_args*) arg;
    enum starpu_task_spawn_mode mode = args->mode;
    int thread_id = args->thread_id;

    // Free the argument structure (no longer needed)
    free(args);

    while (task_completion_counter < task_submitted_counter) {
        pthread_mutex_lock(&task_list->lock);
        struct starpu_task* task = starpu_task_get();
        pthread_mutex_unlock(&task_list->lock);
        if (task) {
            printf("Thread %d spawning task %p\n", thread_id, task);
            starpu_task_spawn(task, mode);
        } else {
            usleep(10);  // Prevent CPU overuse if no tasks are ready
        }
    }

    return NULL;
}

void starpu_task_spawn(struct starpu_task* task, starpu_task_spawn_mode mode) {

    printf("HOST: Task %p spawned\n", task);

    task->self_id = task;

    // Choose a worker (e.g., round-robin)
    int worker_index = task_spawn_counter % num_workers;
    task_spawn_counter++;

    write(workers[worker_index].worker_pipe[1], task, sizeof(struct starpu_task));

    if (mode == REMOTE_PROCESS) {
        // Serialize and send user_data for each handle
        for (int i = 0; i < task->cl->nbuffers; i++) {
            struct starpu_data_handle* handle = task->handles[i];
            printf("Sending data from %p\n", handle);

            // Send the size of the user_data
            size_t data_size = handle->nx * handle->elem_size; // Example: nx * elem_size for array data

            if (write_exact(workers[worker_index].worker_pipe[1], &data_size, sizeof(size_t)) != sizeof(size_t)) {
                perror("Failed to write full data_size");
                return;
            }

            // Send the user_data itself
            if (write_exact(workers[worker_index].worker_pipe[1], handle->user_data, data_size) != data_size) {
                perror("Failed to write full user_data");
                return;
            }
        }
    }
}

void starpu_task_wait_and_spawn(int n_proc, starpu_task_spawn_mode mode) {
    if (mode == REMOTE_PROCESS) {
        pthread_t worker_threads[n_proc];

        for (int i = 0; i < n_proc; i++) {
            struct spawner_args* args = malloc(sizeof(struct spawner_args));
            if (!args) {
                perror("Failed to allocate memory for thread arguments");
                exit(-1);
            }
            args->mode = mode;
            args->thread_id = i;

            if (pthread_create(&worker_threads[i], NULL, task_spawning_worker, args) != 0) {
                perror("Failed to create spawner thread");
                free(args);
                exit(-1);
            }
        }
    
        // Wait for all threads to finish
        for (int i = 0; i < n_proc; i++) {
            pthread_join(worker_threads[i], NULL);
        }
    } else {
        while (task_completion_counter < task_submitted_counter) {
            pthread_mutex_lock(&task_list->lock);
            struct starpu_task* cur = NULL;
            cur = starpu_task_get();

            pthread_mutex_unlock(&task_list->lock);

            if (cur) {
                starpu_task_spawn(cur, mode);
                cur = NULL;
            }
        }
    }
}

void* starpu_arg_init(void* arg1, uint64_t tag_id) {
    struct starpu_func_arg* args = (struct starpu_func_arg*) malloc(sizeof(struct starpu_func_arg));

    if (arg1) {
        args->arg1 = arg1;
    }
    if (tag_id) {
        args->tag_id = tag_id;
    }

    return (void *) args;
}

void starpu_task_run(struct starpu_task* task, int notif_pipe_fd, starpu_task_spawn_mode mode) {
    task->status = TASK_RUNNING;
    struct starpu_codelet* cl = task->cl;

    starpu_cpu_func_t func = cl->cpu_funcs[0];

    void* func_arg = starpu_arg_init(task->cl_arg, task->tag_id);

    func((void *) task->handles, func_arg);

    write(notif_pipe_fd, &task->self_id, sizeof(struct starpu_task*));
    
    if (mode == REMOTE_PROCESS) {
        // Send modified user_data back to the main process
        for (int i = 0; i < cl->nbuffers; i++) {
            struct starpu_data_handle* handle = task->handles[i];

            // Send the size of the user_data
            size_t data_size = handle->nx * handle->elem_size; // Example: nx * elem_size for array data
            if (write_exact(notif_pipe_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
                perror("Failed to write full data_size for notif");
                return;
            }

            // Send the modified user_data
            if (write_exact(notif_pipe_fd, handle->user_data, data_size) != data_size) {
                perror("Failed to write full user_data for notif");
                return;
            }
        }
    }
}