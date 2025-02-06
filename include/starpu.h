#ifndef __STARPU_H__
#define __STARPU_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "starpu_data.h"
#include "starpu_data_interfaces.h"
#include "starpu_helper.h"
#include "starpu_task.h"

#define SHM_NAME "/shm_starpu"
#define SHM_SIZE 4096

#define TYPE float

static struct starpu_task_list task_list;
static struct starpu_data_handle_list data_handle_list;

int starpu_init(void);
void starpu_shutdown(void);
void starpu_create_worker(void);

int worker_pipe[2];
int notification_pipe[2];
int shm_fd;

TYPE* shared_data;

#endif /* __STARPU_H__ */