#ifndef __STARPU_DATA_H__
#define __STARPU_DATA_H__

struct _starpu_data_state {
    enum starpu_data_access_mode current_mode;
    unsigned dimensions;
    void *user_data;
};

typedef struct _starpu_data_state *starpu_data_handle_t;

enum starpu_data_access_mode {
    STARPU_R = (1 << 0), // read-only
    STARPU_W = (1 << 1), // write-only
    STARPU_RW = (STARPU_R | STARPU_W) // read-write
};

#endif /* __STARPU_DATA_H__ */