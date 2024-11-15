#ifndef __STARPU_DATA_FILTERS_H__
#define __STARPU_DATA_FILTERS_H__

#include <starpu.h>
#include <stdarg.h>

struct starpu_data_filter {
    void (*filter_func)(void *parent_interface, void *child_interface, struct starpu_data_filter *, unsigned id, unsigned nparts);

	unsigned nchildren; /**< Number of parts to partition the data into. */
};

#endif