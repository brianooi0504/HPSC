#include "starpu.h"

void starpu_data_handle_submit(struct starpu_data_handle* data_handle) {
	pthread_mutex_lock(&data_handle_list.lock);

    if (data_handle_list.tail) {
        data_handle_list.tail->next_data_handle = data_handle;
    } else {
        data_handle_list.head = data_handle;
    }
    data_handle_list.tail = data_handle;

    pthread_mutex_unlock(&data_handle_list.lock);
}

struct starpu_data_handle* starpu_data_handle_get(void) {
	struct starpu_data_handle* next = NULL;

	pthread_mutex_lock(&data_handle_list.lock);

    if (data_handle_list.head) {
        next = data_handle_list.head;

        data_handle_list.head = next->next_data_handle;

        if (!data_handle_list.head) {
            data_handle_list.tail = NULL;
        }
    }

    pthread_mutex_unlock(&data_handle_list.lock);

   return next;
}

void starpu_data_handle_list_init(struct starpu_data_handle_list *list) {
	pthread_mutex_lock(&list->lock);
	list->head = NULL;
	list->tail = NULL;
}

void starpu_data_handle_init(struct starpu_data_handle *handle) {
    handle->current_mode = STARPU_RW;
    handle->dimensions = 0;
    handle->version_req = -1;
    handle->version_exec = 0;
    handle->nx = 0;
    handle->elem_size = 0;
    handle->user_data = NULL;
    handle->next_data_handle = NULL;
    handle->parent_data_handle = NULL;
    handle->children_data_handles = NULL;
    handle->num_children = 0;
    handle->tag_id = 0;
}