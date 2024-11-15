#include <starpu.h>

struct starpu_data_interface_ops starpu_interface_vector_ops =
{
	.init = vector_init,
	.register_data_handle = register_vector_handle,
	.allocate_data_on_node = allocate_vector_buffer_on_node,
	.to_pointer = vector_to_pointer,
	.free_data_on_node = free_vector_buffer_on_node,
	.cache_data_on_node = cache_vector_buffer_on_node,
	.reuse_data_on_node = reuse_vector_buffer_on_node,
	.map_data = map_vector,
	.unmap_data = unmap_vector,
	.update_map = update_map,
	.copy_methods = &vector_copy_data_methods_s,
	.get_size = vector_interface_get_size,
	.get_alloc_size = vector_interface_get_alloc_size,
	.footprint = footprint_vector_interface_crc32,
	.alloc_footprint = alloc_footprint_vector_interface_crc32,
	.compare = vector_compare,
	.alloc_compare = vector_alloc_compare,
	.interfaceid = STARPU_VECTOR_INTERFACE_ID,
	.interface_size = sizeof(struct starpu_vector_interface),
	.display = display_vector_interface,
	.pack_data = pack_vector_handle,
	.peek_data = peek_vector_handle,
	.unpack_data = unpack_vector_handle,
	.describe = describe,
	.name = "STARPU_VECTOR_INTERFACE",
	.pack_meta = NULL,
	.unpack_meta = NULL,
	.free_meta = NULL
};

/* declare a new data with the vector interface */
void starpu_vector_data_register_allocsize(starpu_data_handle_t *handleptr, int home_node, uintptr_t ptr, size_t nx, size_t elemsize, size_t allocsize)
{
	struct starpu_vector_interface vector =
	{
		.id = STARPU_VECTOR_INTERFACE_ID,
		.ptr = ptr,
		.nx = nx,
		.elemsize = elemsize,
		.dev_handle = ptr,
		.slice_base = 0,
		.offset = 0,
		.allocsize = allocsize,
	};

	if (home_node >= 0)
		starpu_check_on_node(home_node, ptr, nx*elemsize);

	starpu_data_register(handleptr, home_node, &vector, &starpu_interface_vector_ops);
}

void starpu_vector_data_register(starpu_data_handle_t *handleptr, int home_node, uintptr_t ptr, size_t nx, size_t elemsize)
{
	starpu_vector_data_register_allocsize(handleptr, home_node, ptr, nx, elemsize, nx * elemsize);
}



