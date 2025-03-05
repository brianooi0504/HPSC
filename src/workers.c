#include "starpu.h"

int shm_fd;
int task_completion_counter;
int task_spawn_counter;
int task_submitted_counter;

struct starpu_worker* workers;
int num_workers;

TYPE* shared_data;
shm_allocator_t *allocator;

struct starpu_task_list* task_list;
struct starpu_data_handle_list* data_handle_list;

// pthread function to listen to notification pipe
void* notification_listener(void *arg) {
    struct notification_listener_args* args = (struct notification_listener_args*) arg;
    enum starpu_task_spawn_mode mode = args->mode;
    int notif_pipe_fd = args->notif_pipe_fd;

    // Free the argument structure (no longer needed)
    free(args);

    while (1) {
        struct starpu_task* ret_task;
        read(notif_pipe_fd, &ret_task, sizeof(struct starpu_task*));

        if (!ret_task) {
            perror("Notification for task failed");
            exit(-1);
        }

        if (mode == REMOTE_PROCESS) {
            for (int i = 0; i < ret_task->cl->nbuffers; i++) {
                struct starpu_data_handle* handle = ret_task->handles[i];

                // Read the size of the modified user_data
                size_t data_size;
                if (read_exact(notif_pipe_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
                    perror("Failed to read full data_size for notif");
                    return NULL;
                }

                // Read the modified user_data
                if (read_exact(notif_pipe_fd, handle->user_data, data_size) != data_size) {
                    perror("Failed to read full user_data for notif");
                    return NULL;
                }
            }
        }
        
        for (int i = 0; i < ret_task->cl->nbuffers; i++) {
            ret_task->handles[i]->version_exec++;
        }

        ret_task->status = TASK_FINISHED;

        task_completion_counter++;
        printf("Checkpoint %d\n", task_completion_counter);

        struct starpu_task* task = task_list->head;
        while (task) {
            for (int i = 0; i < task->num_dependencies; i++) {
                if (task->dependencies[i] == ret_task) {
                    task->num_dependencies_met++;

                    // If all dependencies are met, mark as TASK_READY
                    if (task->num_dependencies_met == task->num_dependencies) {
                        task->status = TASK_READY;
                        printf("Task %p is now READY\n", task);
                    }
                }
            }
            task = task->next_task;
        }
    }

    return NULL;
}

int starpu_init(int n_proc, starpu_task_spawn_mode mode) {
    shm_init(&allocator);
    
    task_list = malloc(sizeof(struct starpu_task_list));
    data_handle_list = (struct starpu_data_handle_list*) starpu_malloc(sizeof(struct starpu_data_handle_list), mode);

    starpu_task_list_init(task_list);
    starpu_data_handle_list_init(data_handle_list);

    task_completion_counter = 0;
    task_spawn_counter = 0;
    task_submitted_counter = 0;

    workers = NULL;
    num_workers = 0;
    pthread_t listeners[n_proc];

    for (int i = 0; i < n_proc; i++) {
        starpu_create_worker(mode);

        // Create the thread argument structure
        struct notification_listener_args* args = malloc(sizeof(struct notification_listener_args));
        if (!args) {
            perror("Failed to allocate memory for thread arguments");
            exit(-1);
        }
        args->mode = mode;
        args->notif_pipe_fd = workers[i].notif_pipe[0];

        // Create the notification listener thread and pass the arguments
        if (pthread_create(&listeners[i], NULL, notification_listener, args) != 0) {
            perror("Failed to create notification listener thread");
            free(args);
            exit(-1);
        }
    }

    printf("StarPU initiailized\n");

    return 0;
}

void starpu_create_worker(starpu_task_spawn_mode mode) {
    // Allocate memory for a new worker
    workers = realloc(workers, (num_workers + 1) * sizeof(struct starpu_worker));
    if (!workers) {
        perror("Failed to allocate memory for worker");
        exit(-1);
    }

    if (pipe(workers[num_workers].worker_pipe) == -1) {
        perror("Failed to create worker pipe");
        exit(-1);
    }

    if (pipe(workers[num_workers].notif_pipe) == -1) {
        perror("Failed to create notif pipe");
        exit(-1);
    }

    pid_t pid = fork();

    if (pid == 0) {
        /* Child process */
        printf("CHILD PROCESS %d: created\n", getpid());
        workers[num_workers].pid = getpid();

        close(workers[num_workers].worker_pipe[1]);
        close(workers[num_workers].notif_pipe[0]);

        // Use the worker-specific pipe for communication
        starpu_task_read_and_run(workers[num_workers].worker_pipe[0], workers[num_workers].notif_pipe[1], mode);
    } else if (pid > 0) {
        /* Parent process */
        close(workers[num_workers].worker_pipe[0]);
        close(workers[num_workers].notif_pipe[1]);

        num_workers++;
    } else {
        perror("Failed to fork");
        exit(-1);
    }
}

void shm_init(shm_allocator_t **allocator) {
    shm_unlink(SHM_NAME);

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1) {
        perror("shm_open");
        exit(-1);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        exit(-1);
    }

    shared_data = (TYPE *)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }

    close(shm_fd);

    // Set up allocator at the beginning of shared memory
    *allocator = (shm_allocator_t *)shared_data;
    (*allocator)->total_size = SHM_SIZE - sizeof(shm_allocator_t);
    
    // Initialize the free list with the entire available memory
    (*allocator)->free_list = (Block *)((char *)shared_data + sizeof(shm_allocator_t));
    (*allocator)->free_list->size = (*allocator)->total_size - sizeof(Block);
    (*allocator)->free_list->next = NULL;
    (*allocator)->free_list->free = 1;

}

// Find a free block or split one
void* shm_alloc(shm_allocator_t *allocator, size_t size) {
    Block *current = allocator->free_list, *prev = NULL;

    while (current) {
        if (current->free && current->size >= size) {
            // Split block if large enough
            if (current->size >= size + sizeof(Block)) {
                Block *new_block = (Block *)((char *)current + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->next = current->next;
                new_block->free = 1;

                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return (char *)current + sizeof(Block);
        }
        prev = current;
        current = current->next;
    }

    return NULL;  // No sufficient memory available
}

// Free memory and merge adjacent blocks
void shm_free(shm_allocator_t *allocator, void *ptr) {
    if (!ptr) return;

    Block *block = (Block *)((char *)ptr - sizeof(Block));
    block->free = 1;

    // Merge adjacent free blocks
    Block *current = allocator->free_list;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += current->next->size + sizeof(Block);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

// Destroy shared memory
void shm_destroy(void *shm_base, size_t shm_size) {
    munmap(shm_base, shm_size);
    shm_unlink(SHM_NAME);
}

void starpu_shutdown(void) {
    shm_destroy(shared_data, SHM_SIZE);

    printf("StarPU shut down\n");
}