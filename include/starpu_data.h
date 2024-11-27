#ifndef __STARPU_DATA_H__
#define __STARPU_DATA_H__

#include "starpu.h"

enum starpu_data_access_mode {
    STARPU_R = (1 << 0), // read-only
    STARPU_W = (1 << 1), // write-only
    STARPU_RW = (STARPU_R | STARPU_W) // read-write
};

struct starpu_data_handle {
    enum starpu_data_access_mode current_mode;
    unsigned dimensions;
    int version;
    int exec_version;
    size_t nx;
    size_t elem_size;
    void* user_data;
    void** ptrs; // Array of pointers for each dimension
    struct starpu_data_handle* next_data_handle;
};

struct starpu_data_handle_list {
    pthread_mutex_t lock;
    struct starpu_data_handle* head;
    struct starpu_data_handle* tail;
};

void starpu_data_handle_submit(struct starpu_data_handle* data_handle);
struct starpu_data_handle* starpu_data_handle_get(void);

void starpu_data_handle_list_init(struct starpu_data_handle_list *list);

#endif /* __STARPU_DATA_H__ */