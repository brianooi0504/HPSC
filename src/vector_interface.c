#include "starpu.h"

void starpu_vector_data_register(struct starpu_data_handle* handle, size_t n_dim, uintptr_t ptr, size_t nx, size_t elem_size) {
    handle->dimensions = n_dim;
    handle->nx = nx;
    handle->elem_size = elem_size;       
    handle->user_data = (void*)ptr;              
    handle->next_data_handle = NULL;

    starpu_data_handle_submit(handle);
}

struct starpu_data_handle* starpu_data_get_sub_data(struct starpu_data_handle parent_handle, size_t block_index, size_t num_blocks) {
    // Calculate size of each block
    size_t nb_elements = parent_handle.nx / num_blocks;
    size_t offset = block_index * nb_elements * parent_handle.elem_size;

    // Create a sub-handle
    struct starpu_data_handle* sub_handle = malloc(sizeof(struct starpu_data_handle));
    if (!sub_handle) {
        fprintf(stderr, "Memory allocation failed in starpu_data_get_sub_data\n");
        exit(EXIT_FAILURE);
    }

    sub_handle->dimensions = parent_handle.dimensions;
    sub_handle->nx = parent_handle.nx / num_blocks;
    sub_handle->elem_size = parent_handle.elem_size;
    sub_handle->user_data = (void*)((uintptr_t)parent_handle.user_data + offset);
    sub_handle->next_data_handle = NULL;  // Not linked yet

    starpu_data_handle_submit(sub_handle);

    return sub_handle;
}
