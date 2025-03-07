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
#include "starpu_worker.h"
#include "starpu_enum.h"

#define SHM_NAME "/shm_starpu"
#define SHM_SIZE (1 << 30)

#define TYPE float

typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
} Block;

typedef struct {
    size_t total_size;
    Block *free_list;
} shm_allocator_t;

int starpu_init(int n_proc, starpu_task_spawn_mode mode);
void starpu_shutdown(void);
void starpu_create_worker(starpu_task_spawn_mode mode);
void starpu_check(void);

extern int shm_fd;
extern int task_completion_counter;
extern int task_spawn_counter;
extern int task_submitted_counter;

extern struct starpu_task_list* task_list;
extern struct starpu_data_handle_list* data_handle_list;

extern TYPE* shared_data;
extern shm_allocator_t *allocator;

void shm_init(shm_allocator_t **allocator);
void* shm_alloc(shm_allocator_t *allocator, size_t size);
void shm_free(shm_allocator_t *allocator, void *ptr);
void shm_destroy(void *shm_base, size_t shm_size);

#endif /* __STARPU_H__ */