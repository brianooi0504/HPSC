#ifndef __STARPU_H__
#define __STARPU_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "starpu_data.h"
#include "starpu_data_interfaces.h"
#include "starpu_helper.h"
#include "starpu_task.h"

#define SHM_NAME "/shm_starpu"
#define SHM_SIZE (1 << 30)

#define TYPE float

static struct starpu_task_list task_list;
static struct starpu_data_handle_list data_handle_list;

typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
} Block;

typedef struct {
    size_t total_size;
    Block *free_list;
} shm_allocator_t;

int starpu_init(int n_proc);
void starpu_shutdown(void);
void starpu_create_worker(void);
void starpu_check(void);

int worker_pipe[2];
int notification_pipe[2];
int shm_fd;
int task_completion_counter;
int task_spawn_counter;

TYPE* shared_data;
shm_allocator_t *allocator;

void shm_init(shm_allocator_t **allocator);
void* shm_alloc(shm_allocator_t *allocator, size_t size);
void shm_free(shm_allocator_t *allocator, void *ptr);
void shm_destroy(void *shm_base, size_t shm_size);

#endif /* __STARPU_H__ */