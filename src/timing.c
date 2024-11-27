#include <time.h>
#include "starpu.h"

double starpu_timing_now(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    return  (1000000.0*now.tv_sec) + (0.001*now.tv_nsec);
}