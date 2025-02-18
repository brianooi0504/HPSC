#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "starpu.h"

#define MATMULT // INCR, AXPY, STENCIL, MATMULT

// #define N (64*1024*1024)
#define N 1024
#define NBLOCKS 16

#define NPROC 8

#define PRINTARRAY 0
#define CHECK 0

TYPE *_vec_x, *_vec_y, *_vec_z;
TYPE _alpha = 3.4;

struct starpu_data_handle _handle_y, _handle_x, _handle_z;

#ifdef AXPY
#define NDIM 1
void axpy(void *arrays[], void *arg)
{
    printf("Running axpy_cpu function\n");
	TYPE alpha = *((TYPE *)((struct starpu_func_arg*) arg)->arg1);

	// TYPE *block_x = ((struct starpu_data_handle*) arrays[0])->user_data;
    // TYPE *block_y = ((struct starpu_data_handle*) arrays[1])->user_data;

    TYPE *block_x = ((TYPE *) arrays[0]);
    TYPE *block_y = ((TYPE *) arrays[1]);

    printf("block_x: %p\n", block_x);
    printf("block_y: %p\n", block_y);

    int block_size = N/NBLOCKS;

    if (PRINTARRAY) {
        printf("Before: ");
        for (int i = 0; i < block_size; i++) {
            printf(" %.1f ", block_y[i]);
        }
        printf("\n");
    }

    for (int i = 0; i < block_size; i++) {
        block_y[i] = alpha*block_x[i] + block_y[i];
    }

    if (PRINTARRAY) {
        printf("After:  ");
        for (int i = 0; i < block_size; i++) {
            printf(" %.1f ", block_y[i]);
        }
        printf("\n");
    }
    
}

static struct starpu_codelet axpy_cl = {
    .cpu_funcs = {axpy},
    .cpu_funcs_name = {"axpy_cpu"},
    .nbuffers = 2,
    .modes = {STARPU_R, STARPU_RW}
};
#endif

#ifdef INCR
#define NDIM 1
void increment(void *array[], void* arg) {
    printf("Running increment function\n");
    TYPE alpha = *((TYPE *)((struct starpu_func_arg*) arg)->arg1);
    TYPE* block = ((TYPE *) array[0]);
    int block_size = N/NBLOCKS;

    if (PRINTARRAY) {
        printf("Before: ");
        for (int i = 0; i < block_size; i++) {
            printf(" %.1f ", block[i]);
        }
        printf("\n");
    }

    for (int i = 0; i < block_size; i++) {
        block[i] += alpha;
    }

    if (PRINTARRAY) {
        printf("After:  ");
        for (int i = 0; i < block_size; i++) {
            printf(" %.1f ", block[i]);
        }
        printf("\n");
    }
    
}

static struct starpu_codelet increment_cl = {
    .cpu_funcs = {increment},
    .cpu_funcs_name = {"increment"},
    .nbuffers = 1,
    .modes = {STARPU_RW}
};
#endif

#ifdef STENCIL
#define NDIM 2
#define FILTER_SIZE 3

void print_grid(float *grid, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%6.2f ", grid[i * size + j]);
        }
        printf("\n");
    }
}

void stencil(void *arrays[], void *arg) {
    printf("Running stencil function\n");

	TYPE *filter = (TYPE *)((struct starpu_func_arg*) arg)->arg1;
    uint64_t tag_id = ((struct starpu_func_arg*) arg)->tag_id;

	TYPE *input = ((TYPE*) arrays[0]);
    TYPE *output = ((TYPE*) arrays[1]);    

    int block_dim = sqrt(NBLOCKS); // block_dim x block_dim grid
    int block_size = (N/block_dim); // Each process handles a block_size x block_size block

    // Calculate which 32x32 block this process is responsible for
    int block_row = tag_id / block_dim;
    int block_col = tag_id % block_dim;

    // Determine the starting index in the global input/output arrays
    int start_i = block_row * block_size;
    int start_j = block_col * block_size;

    printf("Process %d %lu handling block at row %d, col %d (Start index: [%d,%d])\n",
           getpid(), tag_id, block_row, block_col, start_i, start_j);

    print_grid(filter, FILTER_SIZE);

    // Apply the stencil within the assigned block
    for (int i = 1; i < block_size - 1; i++) {         // Avoid boundary cells
        for (int j = 1; j < block_size - 1; j++) {     // Avoid boundary cells
            float sum = 0.0f;

            // Apply the 3x3 filter
            for (int fi = -1; fi <= 1; fi++) {
                for (int fj = -1; fj <= 1; fj++) {
                    int global_i = start_i + i + fi;
                    int global_j = start_j + j + fj;
                    int input_index = global_i * N + global_j;
                    int filter_index = (fi + 1) * FILTER_SIZE + (fj + 1);

                    sum += input[input_index] * filter[filter_index];
                }
            }

            // Store the result in the output grid
            int output_index = (start_i + i) * N + (start_j + j);
            output[output_index] = sum;
        }
    }
}

float _filter[FILTER_SIZE * FILTER_SIZE] = {
    0.1, 0.8, 0.1,
    0.1, 0.2, 0.1,
    0.1, 0.1, 0.1
};

static struct starpu_codelet stencil_cl = {
    .cpu_funcs = {stencil},
    .cpu_funcs_name = {"stencil"},
    .nbuffers = 2,
    .modes = {STARPU_R, STARPU_RW}
};
#endif

#ifdef MATMULT
#define NDIM 2

void print_grid(float *grid, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%6.2f ", grid[i * size + j]);
        }
        printf("\n");
    }
}

void matmult(void *arrays[], void *arg) {
    printf("Running matrix multiplication function\n");

    TYPE *A = ((TYPE *) arrays[0]);
    TYPE *B = ((TYPE *) arrays[1]);
    TYPE *C = ((TYPE *) arrays[2]);
    uint64_t tag_id = ((struct starpu_func_arg*) arg)->tag_id;

    int block_dim = sqrt(NBLOCKS); // block_dim x block_dim grid
    int block_size = (N/block_dim); // Each process handles a block_size x block_size block

    // Compute which block this process is handling
    int block_row = tag_id / block_dim;
    int block_col = tag_id % block_dim;

    // Compute start index in global matrices
    int start_i = block_row * block_size;
    int start_j = block_col * block_size;

    printf("Process %lu computing block C[%d:%d, %d:%d]\n",
           tag_id, start_i, start_i + block_size, start_j, start_j + block_size);

    // Compute the sub-block of C
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < block_size; j++) {
            float sum = 0.0f;

            // Standard matrix multiplication for a single element
            for (int k = 0; k < N; k++) {
                int a_index = (start_i + i) * N + k;  // Row-major order for A
                int b_index = k * N + (start_j + j);  // Column-major order for B
                sum += A[a_index] * B[b_index];
            }

            // Store the result in C
            int c_index = (start_i + i) * N + (start_j + j);
            C[c_index] = sum;
        }
    }
}

static struct starpu_codelet matmult_cl = {
    .cpu_funcs = {matmult},
    .cpu_funcs_name = {"matmult"},
    .nbuffers = 3,
    .modes = {STARPU_R, STARPU_R, STARPU_RW}
};
#endif

// checking function to ensure final results is correct
void starpu_check(void) {
    #ifdef AXPY
    float maxError = 0.0f;
    for (int i = 0; i < N; i++){
        maxError = fmax(maxError, fabs(_vec_y[i] - 7.4));
    }
    printf("Max error: %f\n", maxError);
    #endif

    #ifdef MATMULT
    //check if matrix _vec_x multiplied by _vec_y equals _vec_z
    float maxError = 0.0f;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                sum += _vec_x[i * N + k] * _vec_y[k * N + j];
            }
            maxError = fmax(maxError, fabs(sum - _vec_z[i * N + j]));
        }
    }
    printf("Max error: %f\n", maxError);
    #endif
}

int main(void) {
    int ret, exit_value = 0;

    ret = starpu_init(NPROC);
	if (ret == -1) {
        exit(-1);
    }

    #ifdef AXPY
    _vec_x = starpu_malloc(N*sizeof(TYPE));
    _vec_y = starpu_malloc(N*sizeof(TYPE));
    #endif

    #ifdef INCR
    _vec_y = starpu_malloc(N*sizeof(TYPE));
    #endif

    #ifdef STENCIL
    _vec_x = starpu_malloc(N*N*sizeof(TYPE));
    _vec_y = starpu_malloc(N*N*sizeof(TYPE));
    #endif

    #ifdef MATMULT
    _vec_x = starpu_malloc(N*N*sizeof(TYPE));
    _vec_y = starpu_malloc(N*N*sizeof(TYPE));
    _vec_z = starpu_malloc(N*N*sizeof(TYPE));
    #endif

    #ifdef AXPY
    for (int i = 0; i < N; i++) {
        _vec_x[i] = 1.0f;
        _vec_y[i] = 4.0f;
    }
    #endif

    #ifdef INCR
    for (int i = 0; i < N; i++) {
        _vec_y[i] = 4.0f;
    }
    #endif

    #ifdef STENCIL
    for (int i = 0; i < N*N; i++) {
        _vec_x[i] = (TYPE) (i+1);
        _vec_y[i] = 0.0f;
    }
    #endif

    #ifdef MATMULT
    for (int i = 0; i < N*N; i++) {
        _vec_x[i] = (TYPE) (i+1);
        _vec_y[i] = 2.0f;
        _vec_z[i] = 0.0f;
    }
    #endif

    if (PRINTARRAY) {
        #ifdef AXPY
        for (int i = 0; i < N; i++) {
            printf("%.1f ", _vec_y[i]);
        }
        printf("\n");
        #endif

        #ifdef STENCIL
        print_grid(_vec_x, N);
        #endif

        #ifdef MATMULT
        print_grid(_vec_x, N);
        print_grid(_vec_y, N);
        #endif
    }

    /* Declare the data to StarPU */
    #ifdef AXPY
	starpu_vector_data_register(&_handle_x, NDIM, (uintptr_t)_vec_x, N, sizeof(TYPE));
    starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N, sizeof(TYPE));
    #endif

    #ifdef INCR
    starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N, sizeof(TYPE));
    #endif

    #ifdef STENCIL
    starpu_vector_data_register(&_handle_x, NDIM, (uintptr_t)_vec_x, N*N, sizeof(TYPE));
    starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N*N, sizeof(TYPE));
    #endif

    #ifdef MATMULT
    starpu_vector_data_register(&_handle_x, NDIM, (uintptr_t)_vec_x, N*N, sizeof(TYPE));
    starpu_vector_data_register(&_handle_y, NDIM, (uintptr_t)_vec_y, N*N, sizeof(TYPE));
    starpu_vector_data_register(&_handle_z, NDIM, (uintptr_t)_vec_z, N*N, sizeof(TYPE));
    #endif

    double start, end;
    start = starpu_timing_now();

    unsigned b;
    for (b = 0; b < NBLOCKS; b++) {
        #ifdef AXPY
        struct starpu_task* task = starpu_task_create(&axpy_cl, &_alpha, sizeof(_alpha), b);
        
        task->handles[0] = starpu_data_get_sub_data(&_handle_x, b, NBLOCKS, NDIM);
		task->handles[1] = starpu_data_get_sub_data(&_handle_y, b, NBLOCKS, NDIM); 
        task->data_pointers[0] = task->handles[0]->user_data;
        task->data_pointers[1] = task->handles[1]->user_data;

        task->version_req[0] = task->handles[0]->version_req + 1;
        task->version_req[1] = task->handles[1]->version_req + 1;
        task->handles[0]->version_req++;
        task->handles[1]->version_req++;
        #endif

        #ifdef INCR
        struct starpu_task* task = starpu_task_create(&increment_cl, &_alpha, sizeof(_alpha), b);
        
        task->handles[0] = starpu_data_get_sub_data(&_handle_y, b, NBLOCKS, NDIM);
        task->data_pointers[0] = task->handles[0]->user_data;
        task->version_req[0] = task->handles[0]->version_req + 1;
        task->handles[0]->version_req++;
        #endif

        #ifdef STENCIL
        struct starpu_task* task = starpu_task_create(&stencil_cl, &_filter, sizeof(_filter), b);
        
        task->handles[0] = starpu_data_get_sub_data(&_handle_x, b, NBLOCKS, NDIM);
		task->handles[1] = starpu_data_get_sub_data(&_handle_y, b, NBLOCKS, NDIM);
        task->data_pointers[0] = task->handles[0]->user_data;
        task->data_pointers[1] = task->handles[1]->user_data; 

        task->version_req[0] = task->handles[0]->version_req + 1;
        task->version_req[1] = task->handles[1]->version_req + 1;
        task->handles[0]->version_req++;
        task->handles[1]->version_req++;
        #endif

        #ifdef MATMULT
        struct starpu_task* task = starpu_task_create(&matmult_cl, &_alpha, sizeof(_alpha), b);
        
        task->handles[0] = starpu_data_get_sub_data(&_handle_x, b, NBLOCKS, NDIM);
		task->handles[1] = starpu_data_get_sub_data(&_handle_y, b, NBLOCKS, NDIM); 
        task->handles[2] = starpu_data_get_sub_data(&_handle_z, b, NBLOCKS, NDIM); 
        task->data_pointers[0] = task->handles[0]->user_data;
        task->data_pointers[1] = task->handles[1]->user_data;
        task->data_pointers[2] = task->handles[2]->user_data;

        task->version_req[0] = task->handles[0]->version_req + 1;
        task->version_req[1] = task->handles[1]->version_req + 1;
        task->version_req[2] = task->handles[2]->version_req + 1;
        task->handles[0]->version_req++;
        task->handles[1]->version_req++;
        task->handles[2]->version_req++;
        #endif

        starpu_task_submit(task); // add the task to the task list
        task_spawn_counter++;

    }

    #ifdef AXPY
    // struct starpu_task* task = starpu_task_create(&axpy_cl, &_alpha, sizeof(_alpha), 1);
    // task->handles[0] = starpu_data_get_sub_data(&_handle_x, 1, NBLOCKS, NDIM);
    // task->handles[1] = starpu_data_get_sub_data(&_handle_y, 1, NBLOCKS, NDIM);
    // task->data_pointers[0] = task->handles[0]->user_data;
    // task->data_pointers[1] = task->handles[1]->user_data;
    // task->version_req[0] = 2;
    // task->version_req[1] = 2;
    // task->handles[0]->version_req = 2;
    // task->handles[1]->version_req = 2;
    // starpu_task_submit(task);
    // task_spawn_counter++;

    // struct starpu_task* task2 = starpu_task_create(&axpy_cl, &_alpha, sizeof(_alpha), 1);
    // task2->handles[0] = starpu_data_get_sub_data(&_handle_x, 1, NBLOCKS, NDIM);
    // task2->handles[1] = starpu_data_get_sub_data(&_handle_y, 1, NBLOCKS, NDIM);
    // task2->data_pointers[0] = task2->handles[0]->user_data;
    // task2->data_pointers[1] = task2->handles[1]->user_data;
    // task2->version_req[0] = 1;
    // task2->version_req[1] = 1;
    // task2->handles[0]->version_req = 1;
    // task2->handles[1]->version_req = 1;
    // starpu_task_submit(task2);
    // task_spawn_counter++;
    #endif

    #ifdef INCR
    // struct starpu_task* task = starpu_task_create(&increment_cl, &_alpha, sizeof(_alpha), 1);
    // task->handles[0] = starpu_data_get_sub_data(&_handle_y, 1, NBLOCKS, NDIM);
    // task->data_pointers[0] = task->handles[0]->user_data;
    // task->version_req[0] = 2;
    // task->handles[0]->version_req = 2;
    // starpu_task_submit(task);
    // task_spawn_counter++;

    // struct starpu_task* task2 = starpu_task_create(&increment_cl, &_alpha, sizeof(_alpha), 1);
    // task2->handles[0] = starpu_data_get_sub_data(&_handle_y, 1, NBLOCKS, NDIM);
    // task2->data_pointers[0] = task->handles[0]->user_data;
    // task2->version_req[0] = 1;
    // task2->handles[0]->version_req = 1;
    // starpu_task_submit(task2);
    // task_spawn_counter++;
    #endif

    starpu_task_wait_and_spawn(); // executes all the tasks in the task list

    end = starpu_timing_now();
    double timing = end - start; // us (microsecs)

    printf("Time elapsed: %.2fus\n", timing);

    if (PRINTARRAY) {
        #ifdef AXPY
        for (int i = 0; i < N; i++) {
            printf("%.1f ", _vec_y[i]);
        }
        printf("\n");
        #endif

        #ifdef INCR
        for (int i = 0; i < N; i++) {
            printf("%.1f ", _vec_y[i]);
        }
        printf("\n");
        #endif

        #ifdef STENCIL
        print_grid(_vec_y, N);
        #endif

        #ifdef MATMULT
        print_grid(_vec_z, N);
        #endif
    }

    /* Stop StarPU */
    if (CHECK) {
        starpu_check();
    }

    starpu_shutdown();
    return exit_value;
}