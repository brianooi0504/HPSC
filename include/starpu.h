#ifndef __STARPU_H__
#define __STARPU_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "starpu_data.h"
#include "starpu_data_interfaces.h"
#include "starpu_helper.h"
#include "starpu_task.h"

static struct starpu_task_list task_list;
static struct starpu_data_handle_list data_handle_list;

int starpu_init(void);
void starpu_shutdown(void);

#endif /* __STARPU_H__ */