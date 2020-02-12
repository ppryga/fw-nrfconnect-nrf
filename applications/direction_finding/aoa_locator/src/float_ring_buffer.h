#ifndef AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_
#define AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_

#include <stddef.h>
#include <stdbool.h>

#define BUFFER_SIZE 20


struct float_ring_buffer {
	float buffer[BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t max_len;
};

 struct float_ring_buffer_iter {
	struct float_ring_buffer *buf;
	int idx;
};

void ring_buffer_reset(struct float_ring_buffer *buf);
void ring_buffer_init(struct float_ring_buffer *buf);

bool ring_buffer_is_full(struct float_ring_buffer *buf);
bool ring_buffer_is_empty(struct float_ring_buffer *buf);
size_t ring_buffer_size(struct float_ring_buffer *buf);
size_t ring_buffer_len(struct float_ring_buffer *buf);

void ring_buffer_push(struct float_ring_buffer *buf, float data);
int ring_buffer_pop(struct float_ring_buffer *buf, float *data);

int ring_buffer_get_iterator(struct float_ring_buffer *buf,
			     struct float_ring_buffer_iter *iter);

//iter procedures
int ring_buffer_iter_advance(struct float_ring_buffer_iter *iter);
int ring_buffer_iter_reset(struct float_ring_buffer_iter *iter);
bool ring_buffer_iter_is_end(struct float_ring_buffer_iter *iter);
int ring_buffer_iter_get(struct float_ring_buffer_iter *iter, float *data);
int ring_buffer_iter_read(struct float_ring_buffer_iter *iter, float *data);

#endif /* AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_ */
