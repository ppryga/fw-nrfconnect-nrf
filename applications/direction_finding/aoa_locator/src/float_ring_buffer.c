#include "float_ring_buffer.h"
#include <errno.h>
#include <string.h>
#include <assert.h>

static size_t advance_pointer(size_t ptr, size_t max_size)
{
	return ((ptr + 1) < max_size) ? (ptr+1) : 0;
}

void ring_buffer_reset(struct float_ring_buffer *buf)
{
	assert(buf != NULL);
	buf->tail = 0;
	buf->head = 0;
	memset(buf, 0, buf->max_len * sizeof(float));
}

void ring_buffer_init(struct float_ring_buffer *buf)
{
	assert(buf != NULL);
	buf->max_len = FR_INTERNAL_BUFFER_SIZE;
	ring_buffer_reset(buf);
}

bool ring_buffer_is_full(struct float_ring_buffer *buf)
{
	assert(buf != NULL);
	size_t head = advance_pointer(buf->head, buf->max_len);;

	return (buf->tail == head);
}

bool ring_buffer_is_empty(struct float_ring_buffer *buf)
{
	return (buf->tail == buf->head);
}

size_t ring_buffer_max_len(struct float_ring_buffer *buf)
{
	assert(buf != NULL);
	return buf->max_len-1;
}

size_t ring_buffer_len(struct float_ring_buffer *buf)
{
	assert(buf != NULL);
	int len;

	if (buf->head >= buf->tail) {
		len = (buf->head - buf->tail);
	} else {
		len = ((buf->max_len - buf->tail) + buf->head);
	}

	return len;
}

void ring_buffer_push(struct float_ring_buffer *buf, float data)
{
	assert(buf != NULL);
	buf->buffer[buf->head] = data;

	buf->head = advance_pointer(buf->head, buf->max_len);

	if(buf->head == buf->tail) {
		buf->tail = advance_pointer(buf->tail, buf->max_len);
	}
}

int ring_buffer_pop(struct float_ring_buffer *buf, float *data)
{
	if (buf == NULL || data == NULL) {
		return -EINVAL;
	}

	int err;

	if (ring_buffer_is_empty(buf)) {
		err = -ENODATA;
	} else {
		*data = buf->buffer[buf->tail];
		buf->tail = advance_pointer(buf->tail, buf->max_len);
		err = 0;
	}

	return err;
}


int ring_buffer_get_iterator(struct float_ring_buffer *buf,
			     struct float_ring_buffer_iter *iter)
{
	if ( buf == NULL || iter == NULL) {
		return -EINVAL;
	}

	iter->buf = buf;
	iter->idx = buf->tail;

	return 0;
}

int ring_buffer_iter_advance(struct float_ring_buffer_iter *iter)
{
	if (iter == NULL) {
		return -EINVAL;
	}

	iter->idx = (iter->idx + 1) % iter->buf->max_len;

	return 0;
}

int ring_buffer_iter_reset(struct float_ring_buffer_iter *iter)
{
	if (iter == NULL) {
		return -EINVAL;
	}

	iter->idx = iter->buf->tail;

	return 0;
}

bool ring_buffer_iter_is_end(struct float_ring_buffer_iter *iter)
{
	assert(iter != NULL);
	if (iter->idx == iter->buf->head) {
		return true;
	} else {
		return false;
	}
}

int ring_buffer_iter_get(struct float_ring_buffer_iter *iter, float *data)
{
	if (iter == NULL || data == NULL) {
		return -EINVAL;
	}

	int err = ring_buffer_iter_read(iter, data);
	if (!err) {
		err = ring_buffer_iter_advance(iter);
	}

	return err;
}

int ring_buffer_iter_read(struct float_ring_buffer_iter *iter, float *data)
{
	if (iter == NULL || data == NULL) {
		return -EINVAL;
	}

	*data = iter->buf->buffer[iter->idx];

	return 0;
}
