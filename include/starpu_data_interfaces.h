#ifndef __STARPU_DATA_INTERFACES_H__
#define __STARPU_DATA_INTERFACES_H__

#include "starpu.h"
#include "starpu_enum.h"

void starpu_vector_data_register(struct starpu_data_handle* handle, size_t n_dim, uintptr_t ptr, size_t nx, size_t elem_size);

struct starpu_data_handle* starpu_data_get_sub_data(struct starpu_data_handle* parent_handle, size_t block_index, size_t num_blocks, size_t n_dim, starpu_task_spawn_mode mode);

void* starpu_malloc(size_t t, starpu_task_spawn_mode mode);

#endif /* __STARPU_DATA_INTERFACES_H__ */