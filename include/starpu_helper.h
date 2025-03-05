#ifndef __STARPU_HELPER_H__
#define __STARPU_HELPER_H__

#include "starpu.h"

double starpu_timing_now(void);
ssize_t read_exact(int fd, void *buf, size_t size);
ssize_t write_exact(int fd, const void *buf, size_t size);

#endif // __STARPU_HELPER_H__