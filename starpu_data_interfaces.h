#ifndef __STARPU_DATA_INTERFACES_H__
#define __STARPU_DATA_INTERFACES_H__

#include <starpu_data.h>
#include <stdio.h>

enum starpu_data_interface_id
{
	STARPU_UNKNOWN_INTERFACE_ID	= -1, /**< Unknown interface */
	STARPU_MATRIX_INTERFACE_ID	= 0,  /**< Identifier for the matrix data interface */
	STARPU_BLOCK_INTERFACE_ID	= 1,  /**< Identifier for the block data interface*/
	STARPU_VECTOR_INTERFACE_ID	= 2,  /**< Identifier for the vector data interface*/
	STARPU_CSR_INTERFACE_ID		= 3,  /**< Identifier for the CSR data interface*/
	STARPU_BCSR_INTERFACE_ID	= 4,  /**< Identifier for the BCSR data interface*/
	STARPU_VARIABLE_INTERFACE_ID	= 5,  /**< Identifier for the variable data interface*/
	STARPU_VOID_INTERFACE_ID	= 6,  /**< Identifier for the void data interface*/
	STARPU_MULTIFORMAT_INTERFACE_ID = 7,  /**< Identifier for the multiformat data interface*/
	STARPU_COO_INTERFACE_ID		= 8,  /**< Identifier for the COO data interface*/
	STARPU_TENSOR_INTERFACE_ID	= 9,  /**< Identifier for the tensor data interface*/
	STARPU_NDIM_INTERFACE_ID	= 10, /**< Identifier for the ndim array data interface*/
	STARPU_MAX_INTERFACE_ID		= 11  /**< Maximum number of data interfaces */
};

struct starpu_vector_interface
{
	enum starpu_data_interface_id id; /**< Identifier of the interface */

	uintptr_t ptr;	      /**< local pointer of the vector */
	uintptr_t dev_handle; /**< device handle of the vector. */
	size_t offset;	      /**< offset in the vector */
	size_t nx;	      /**< number of elements on the x-axis of the vector */
	size_t elemsize;      /**< size of the elements of the vector */
	size_t slice_base;  /**< vector slice base, used by the StarPU OpenMP runtime support */
	size_t allocsize;     /**< size actually currently allocated */
};

/**
   Return a pointer to the array designated by interface, valid on CPUs and CUDA only. For OpenCL, the device handle and offset need to be used instead.
 */
#define STARPU_VECTOR_GET_PTR(interface)	(((struct starpu_vector_interface *)(interface))->ptr)

/**
   Return the number of elements registered into the array
   designated by interface.
 */
#define STARPU_VECTOR_GET_NX(interface)		(((struct starpu_vector_interface *)(interface))->nx)

#endif /* __STARPU_DATA_INTERFACES_H__ */