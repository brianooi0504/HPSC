#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "starpu.h"

#define TYPE float

// #define N (16*1024*1024)
#define N 32

#define NBLOCKS	8
#define NDIM 1

TYPE *_vec_x, *_vec_y;
TYPE _alpha = 3.4;

struct starpu_data_handle _handle_y, _handle_x;

void axpy_cpu(void *arrays[], void *arg)
{
    printf("Running axpy_cpu function\n");

	TYPE alpha = *((TYPE *)arg);

	TYPE *block_x = (TYPE *) arrays[0];
    TYPE *block_y = (TYPE *) arrays[1];

    int block_size = N/NBLOCKS;

    printf("Before: ");
    for (int i = 0; i < block_size; i++) {
        printf(" %.1f ", block_y[i]);
    }
    printf("\n");
    

    for (int i = 0; i < block_size; i++) {
        block_y[i] = alpha*block_x[i] + block_y[i];
    }

    printf("After:  ");
    for (int i = 0; i < block_size; i++) {
        printf(" %.1f ", block_y[i]);
    }
    printf("\n");
    
}

static struct starpu_codelet axpy_cl = {
    .cpu_funcs = {axpy_cpu},
    .cpu_funcs_name = {"axpy_cpu"},
    .nbuffers = 2,
    .modes = {STARPU_R, STARPU_RW}
};

int main(void) {
    int ret, exit_value = 0;

    ret = starpu_init();
	if (ret == -1) {
        exit(-1);
    }

    _vec_x = malloc(N*sizeof(TYPE));
    _vec_y = malloc(N*sizeof(TYPE));

    for (int i = 0; i < N; i++) {
        _vec_x[i] = 1.0f;
        _vec_y[i] = 4.0f;
    }

    for (int i = 0; i < N; i++) {
        printf("%.1f ", _vec_x[i]);
    }
    printf("\n");

    for (int i = 0; i < N; i++) {
        printf("%.1f ", _vec_y[i]);
    }
    printf("\n");

    /* Declare the data to StarPU */
	starpu_vector_data_register(&_handle_x, NDIM, (uintptr_t)_vec_x, N, sizeof(TYPE));
	starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N, sizeof(TYPE));

    double start, end;
    start = starpu_timing_now();

    unsigned b;
    for (b = 0; b < NBLOCKS; b++) {
        struct starpu_task* task = starpu_task_create();

        task->cl = &axpy_cl;

        task->cl_arg = &_alpha;
		task->cl_arg_size = sizeof(_alpha);
        
        task->handles[0] = starpu_data_get_sub_data(&_handle_x, b, NBLOCKS);
		task->handles[1] = starpu_data_get_sub_data(&_handle_y, b, NBLOCKS); 

        task->version_req[0] = task->handles[0]->version_req + 1;
        task->version_req[1] = task->handles[1]->version_req + 1;
        task->handles[0]->version_req++;
        task->handles[1]->version_req++;

        task->tag_id = b;

        starpu_task_submit(task); // add the task to the task list

    }

    // Second task to test sub_handle finding
    struct starpu_task* task = starpu_task_create();
    task->cl = &axpy_cl;
    task->cl_arg = &_alpha;
    task->cl_arg_size = sizeof(_alpha);
    task->handles[0] = starpu_data_get_sub_data(&_handle_x, 3, NBLOCKS);
    task->handles[1] = starpu_data_get_sub_data(&_handle_y, 3, NBLOCKS);
    task->version_req[0] = task->handles[0]->version_req + 1;
    task->version_req[1] = task->handles[1]->version_req + 1;
    task->handles[0]->version_req++;
    task->handles[1]->version_req++;
    task->tag_id = 3;
    starpu_task_submit(task);

    starpu_task_wait_for_all(); // executes all the tasks in the task list

    end = starpu_timing_now();
    double timing = end - start; // us (microsecs)

    /* Stop StarPU */
    starpu_shutdown();

    printf("Time elapsed: %.2fus\n", timing);

    for (int i = 0; i < N; i++) {
        printf("%.1f ", _vec_x[i]);
    }
    printf("\n");

    for (int i = 0; i < N; i++) {
        printf("%.1f ", _vec_y[i]);
    }
    printf("\n");

    return exit_value;
}