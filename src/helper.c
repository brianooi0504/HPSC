#include <time.h>
#include "starpu.h"

double starpu_timing_now(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    return  (1000000.0*now.tv_sec) + (0.001*now.tv_nsec);
}

ssize_t read_exact(int fd, void *buf, size_t size) {
    size_t total_read = 0;
    while (total_read < size) {
        ssize_t bytes_read = read(fd, (char*)buf + total_read, size - total_read);
        if (bytes_read <= 0) {
            perror("Failed to read from pipe");
            return -1;
        }
        total_read += bytes_read;
    }
    return total_read;
}

ssize_t write_exact(int fd, const void *buf, size_t size) {
    size_t total_written = 0;
    while (total_written < size) {
        ssize_t bytes_written = write(fd, (char*)buf + total_written, size - total_written);
        if (bytes_written <= 0) {
            perror("Failed to write to pipe");
            return -1;
        }
        total_written += bytes_written;
    }
    return total_written;
}