#ifndef __STARPU_DATA_INTERFACES_H__
#define __STARPU_DATA_INTERFACES_H__

#include "starpu_data.h"

void starpu_vector_data_register(struct starpu_data_handle* handle, size_t n_dim, uintptr_t ptr, size_t nx, size_t elem_size);

struct starpu_data_handle* starpu_data_get_sub_data(struct starpu_data_handle* parent_handle, size_t block_index, size_t num_blocks);

#endif /* __STARPU_DATA_INTERFACES_H__ */