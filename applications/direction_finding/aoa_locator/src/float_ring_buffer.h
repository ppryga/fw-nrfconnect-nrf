#ifndef AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_
#define AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_

#include <stddef.h>
#include <stdbool.h>

#define BUFFER_SIZE 20


typedef struct float_ring_buffer_t {
	float buffer[BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t max_len;
} float_ring_buffer;

typedef struct float_ring_buffer_iter_t {
	float_ring_buffer *buf;
	int idx;
} float_ring_buffer_iter;

void ring_buffer_reset(float_ring_buffer *buf);
void ring_buffer_init(float_ring_buffer *buf);

bool ring_buffer_is_full(float_ring_buffer *buf);
bool ring_buffer_is_empty(float_ring_buffer *buf);
size_t ring_buffer_size(float_ring_buffer *buf);
size_t ring_buffer_len(float_ring_buffer *buf);

void ring_buffer_push(float_ring_buffer *buf, float data);
int ring_buffer_pop(float_ring_buffer *buf, float *data);

int ring_buffer_get_iterator(float_ring_buffer *buf,
						  float_ring_buffer_iter *iter);

//iter procedures
int ring_buffer_iter_advance(float_ring_buffer_iter *iter);
int ring_buffer_iter_reset(float_ring_buffer_iter *iter);
bool ring_buffer_iter_is_end(float_ring_buffer_iter *iter);
int ring_buffer_iter_get(float_ring_buffer_iter *iter, float *data);
int ring_buffer_iter_read(float_ring_buffer_iter *iter, float *data);

#endif /* AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_ */
