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
