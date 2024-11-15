void starpu_data_register(starpu_data_handle_t *handleptr, int home_node, void *data_interface, struct starpu_data_interface_ops *ops)
{
	STARPU_ASSERT_MSG(home_node >= -1 && home_node < (int)starpu_memory_nodes_get_count(), "Invalid memory node number");
	starpu_data_handle_t handle = _starpu_data_handle_allocate(ops, home_node);

	STARPU_ASSERT(handleptr);
	*handleptr = handle;

	if (ops->interfaceid == STARPU_UNKNOWN_INTERFACE_ID)
	{
		ops->interfaceid = starpu_data_interface_get_next_id();
	}

	/* fill the interface fields with the appropriate method */
	STARPU_ASSERT(ops->register_data_handle);
	ops->register_data_handle(handle, home_node, data_interface);

	_starpu_data_register_ops(ops);

	_starpu_register_new_data(handle, home_node, 0);
	_STARPU_TRACE_HANDLE_DATA_REGISTER(handle);
}