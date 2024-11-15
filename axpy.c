#include <starpu.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

// #include "starpu_helper.h"
// #include "starpu_task.h"

#define TYPE float

#define N	(16*1024*1024)

#define NBLOCKS	8

TYPE *_vec_x, *_vec_y;
TYPE _alpha = 3.41;

starpu_data_handle_t _handle_y, _handle_x;

void axpy_cpu(void *descr[], void *arg)
{
	TYPE alpha = *((TYPE *)arg);

	size_t n = STARPU_VECTOR_GET_NX(descr[0]);

	TYPE *block_x = (TYPE *)STARPU_VECTOR_GET_PTR(descr[0]);
	TYPE *block_y = (TYPE *)STARPU_VECTOR_GET_PTR(descr[1]);

	AXPY((int)n, alpha, block_x, 1, block_y, 1);
}

static struct starpu_codelet axpy_cl = {
    .cpu_funcs = {axpy_cpu},
    .cpu_funcs_name = {"axpy_cpu"},
    .nbuffers = 2,
    .modes = {STARPU_R, STARPU_RW}
};

int main(void) {
    int ret, exit_value = 0;

    ret = starpu_init(NULL);
	if (ret == -ENODEV)
		return 77;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

    _vec_x = malloc(N*sizeof(TYPE));
    _vec_y = malloc(N*sizeof(TYPE));

    unsigned i;

    for (int i = 0; i < N; i++) {
        _vec_x[i] = 1.0f;
        _vec_y[i] = 4.0f;
    }

    /* Declare the data to StarPU */
	starpu_vector_data_register(&_handle_x, STARPU_MAIN_RAM, (uintptr_t)_vec_x, N, sizeof(TYPE));
	starpu_vector_data_register(&_handle_y, STARPU_MAIN_RAM, (uintptr_t)_vec_y, N, sizeof(TYPE));

    // /* Divide the vector into blocks */
	// struct starpu_data_filter block_filter =
	// {
	// 	.filter_func = starpu_vector_filter_block,
	// 	.nchildren = NBLOCKS
	// };

    // starpu_data_partition(_handle_x, &block_filter);
	// starpu_data_partition(_handle_y, &block_filter);

    double start, end;
    start = starpu_timing_now();

    unsigned b;
    for (b = 0; b < NBLOCKS; b++) {
        struct starpu_task* task = starpu_task_create();

        task->cl = &axpy_cl;

        task->cl_arg = &_alpha;
		task->cl_arg_size = sizeof(_alpha);
        
        task->handles[0] = starpu_data_get_sub_data(_handle_x, 1, b);
		task->handles[1] = starpu_data_get_sub_data(_handle_y, 1, b); // start_dim0, end_dim0, start_dim1, end_dim1...

        task->tag_id = b;

        ret = starpu_task_submit(task); // add the task to the task list

        if (ret == -ENODEV)
		{
			exit_value = 77;
			goto enodev;
		}
		STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

    }

    starpu_task_wait_for_all(); // executes all the tasks in the task list

// enodev:
// 	starpu_data_unpartition(_handle_x, STARPU_MAIN_RAM);
// 	starpu_data_unpartition(_handle_y, STARPU_MAIN_RAM);
// 	starpu_data_unregister(_handle_x);
// 	starpu_data_unregister(_handle_y);

    end = starpu_timing_now();
    double timing = end - start; // us (microsecs)

    /* Stop StarPU */
    starpu_shutdown();

    return exit_value;
}