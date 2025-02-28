#ifndef __STARPU_DATA_H__
#define __STARPU_DATA_H__

#include "starpu.h"
#include "starpu_enum.h"

struct starpu_data_handle {
    enum starpu_data_access_mode current_mode;
    unsigned dimensions;
    int version_req;
    int version_exec;
    size_t nx;
    size_t elem_size;
    void* user_data;
    struct starpu_data_handle* next_data_handle;
    struct starpu_data_handle* parent_data_handle;
    struct starpu_data_handle** children_data_handles;
    size_t num_children;
    uint64_t tag_id;
};

struct starpu_data_handle_list {
    pthread_mutex_t lock;
    struct starpu_data_handle* head;
    struct starpu_data_handle* tail;
};

void starpu_data_handle_submit(struct starpu_data_handle* data_handle);
struct starpu_data_handle* starpu_data_handle_get(void);

void starpu_data_handle_list_init(struct starpu_data_handle_list *list);

void starpu_data_handle_init(struct starpu_data_handle *handle);

#endif /* __STARPU_DATA_H__ */