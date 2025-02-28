#include "starpu.h"

void starpu_vector_data_register(struct starpu_data_handle* handle, size_t n_dim, uintptr_t ptr, size_t nx, size_t elem_size) {
    starpu_data_handle_init(handle);
    handle->dimensions = n_dim;
    handle->nx = nx;
    handle->elem_size = elem_size;       
    handle->user_data = (void*)ptr;        

    starpu_data_handle_submit(handle);
}

// Find sub_handle by parent_handle and tag_id
// if exists return sub_handle
struct starpu_data_handle* starpu_data_find_sub_data(struct starpu_data_handle* parent_handle, size_t block_index) {
    for (size_t i = 0; i < parent_handle->num_children; i++) {
        struct starpu_data_handle* sub_handle = parent_handle->children_data_handles[i];
        if (sub_handle->tag_id == block_index) {
            return sub_handle;
        }
    }

    return NULL;
}

struct starpu_data_handle* starpu_data_get_sub_data(struct starpu_data_handle* parent_handle, size_t block_index, size_t num_blocks, size_t n_dim, starpu_task_spawn_mode mode) {

    struct starpu_data_handle* sub_handle;

    sub_handle = starpu_data_find_sub_data(parent_handle, block_index);
    if (sub_handle) {
        return sub_handle;
    }
    
    // Calculate size of each block
    size_t nb_elements = parent_handle->nx / num_blocks;
    size_t offset = block_index * nb_elements * parent_handle->elem_size;

    // Create a sub-handle
    sub_handle = (struct starpu_data_handle*) starpu_malloc(sizeof(struct starpu_data_handle), mode);
    if (!sub_handle) {
        fprintf(stderr, "Memory allocation failed in starpu_data_get_sub_data\n");
        exit(EXIT_FAILURE);
    }

    starpu_data_handle_init(sub_handle);
    sub_handle->dimensions = parent_handle->dimensions;
    sub_handle->elem_size = parent_handle->elem_size;
    if (n_dim > 1) {
        sub_handle->nx = parent_handle->nx;
        sub_handle->user_data = (void*)((uintptr_t)parent_handle->user_data);
    } else {
        sub_handle->nx = parent_handle->nx / num_blocks;
        sub_handle->user_data = (void*)((uintptr_t)parent_handle->user_data + offset);
    }
    sub_handle->version_req = parent_handle->version_req;
    sub_handle->version_exec = parent_handle->version_exec;
    sub_handle->parent_data_handle = parent_handle;
    sub_handle->tag_id = block_index;

    parent_handle->children_data_handles = realloc(parent_handle->children_data_handles, (parent_handle->num_children + 1) * sizeof(struct starpu_data_handle*));
    parent_handle->children_data_handles[parent_handle->num_children] = sub_handle;
    parent_handle->num_children++;

    starpu_data_handle_submit(sub_handle);

    return sub_handle;
}