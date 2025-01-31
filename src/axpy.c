/*
    TOASK:
    partitioning: register before or after partitiong
    a data handle per partition or on the fly

    data_handle pointer to each partiiton or array indices

    data_handle versions

    TODO:
    vector_partition()
    task_run()
    axpy_cpu get descr from pointers
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "starpu.h"

#define TYPE float

// #define N (16*1024*1024)
#define N 64

#define NBLOCKS	8
#define NDIM 1

TYPE *_vec_x, *_vec_y, *_arr;
TYPE _alpha = 3.41;

struct starpu_data_handle _handle_y, _handle_x, _handle_arr;

void axpy_cpu(void *descr[], void *arg)
{
	TYPE alpha = *((TYPE *)arg);

	// size_t n = STARPU_VECTOR_GET_NX(descr[0]);

	// TYPE *block_x = (TYPE *)STARPU_VECTOR_GET_PTR(descr[0]);
	// TYPE *block_y = (TYPE *)STARPU_VECTOR_GET_PTR(descr[1]);

	// AXPY((int)n, alpha, block_x, 1, block_y, 1);
    printf("Running axpy_cpu with alpha = %.2f\n", alpha);
}

void increment(void *array[], void* arg) {
    printf("Running increment function\n");
    TYPE* block = (TYPE *) array;
    int block_size = N/NBLOCKS;

    printf("Before: ");
    for (int i = 0; i < block_size; i++) {
        printf(" %.0f ", block[i]);
    }
    printf("\n");
    

    for (int i = 0; i < block_size; i++) {
        block[i]++;
    }

    printf("After:  ");
    for (int i = 0; i < block_size; i++) {
        printf(" %.0f ", block[i]);
    }
    printf("\n");
}

static struct starpu_codelet axpy_cl = {
    .cpu_funcs = {axpy_cpu},
    .cpu_funcs_name = {"axpy_cpu"},
    .nbuffers = 2,
    .modes = {STARPU_R, STARPU_RW}
};

static struct starpu_codelet increment_cl = {
    .cpu_funcs = {increment},
    .cpu_funcs_name = {"increment"},
    .nbuffers = 1,
    .modes = {STARPU_RW}
};

int main(void) {
    int ret, exit_value = 0;

    ret = starpu_init();
	if (ret == -1) {
        exit(-1);
    }

    // _vec_x = malloc(N*sizeof(TYPE));
    // _vec_y = malloc(N*sizeof(TYPE));

    // for (int i = 0; i < N; i++) {
    //     _vec_x[i] = 1.0f;
    //     _vec_y[i] = 4.0f;
    // }

    _arr = malloc(N*sizeof(TYPE));

    for (int i = 0; i < N; i++) {
        _arr[i] = 0.0f;
    }

    for (int i = 10; i < 24; i++) {
        _arr[i] = 1.0f;
    }

    for (int i = 0; i < N; i++) {
        printf("%.0f", _arr[i]);
    }
    printf("\n");

    /* Declare the data to StarPU */
	// starpu_vector_data_register(&_handle_x, NDIM, (uintptr_t)_vec_x, N, sizeof(TYPE));
	// starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N, sizeof(TYPE));
    starpu_vector_data_register(&_handle_arr, NDIM, (uintptr_t) _arr, N, sizeof(TYPE));

    double start, end;
    start = starpu_timing_now();

    unsigned b;
    for (b = 0; b < NBLOCKS; b++) {
        struct starpu_task* task = starpu_task_create();

        task->cl = &increment_cl;

        task->cl_arg = &_alpha;
		task->cl_arg_size = sizeof(_alpha);
        
        // task->handles[0] = starpu_data_get_sub_data(_handle_x, b, NBLOCKS);
		// task->handles[1] = starpu_data_get_sub_data(_handle_y, b, NBLOCKS); // start_dim0, end_dim0, start_dim1, end_dim1...
        task->handles[0] = starpu_data_get_sub_data(&_handle_arr, b, NBLOCKS);
        task->version_req[0] = task->handles[0]->version_req + 1;
        task->handles[0]->version_req++;

        task->tag_id = b;

        starpu_task_submit(task); // add the task to the task list

    }

    // Second task to test sub_handle finding
    // struct starpu_task* task = starpu_task_create();
    // task->cl = &increment_cl;
    // task->cl_arg = &_alpha;
    // task->cl_arg_size = sizeof(_alpha);
    // task->handles[0] = starpu_data_get_sub_data(&_handle_arr, 1, NBLOCKS);
    // task->version_req[0] = task->handles[0]->version_req + 1;
    // task->handles[0]->version_req++;
    // task->tag_id = 1;
    // starpu_task_submit(task);

    starpu_task_wait_and_spawn(); // executes all the tasks in the task list

    end = starpu_timing_now();
    double timing = end - start; // us (microsecs)

    /* Stop StarPU */
    starpu_shutdown();

    printf("Time elapsed: %.2fus\n", timing);

    for (int i = 0; i < N; i++) {
        printf("%.0f", _arr[i]);
    }
    printf("\n");

    sleep(5);

    return exit_value;
}