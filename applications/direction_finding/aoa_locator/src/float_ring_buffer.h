#ifndef AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_
#define AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_

#include <stddef.h>
#include <stdbool.h>

/** @brief Number of items a ring buffer may store
 */
#define FR_INTERNAL_BUFFER_SIZE 20

/** @brief Complex number data structure
 */
struct complex {
	float real;
	float imag;
};

/** @brief Ring buffer data structure
 */
struct float_ring_buffer {
	struct complex buffer[FR_INTERNAL_BUFFER_SIZE];
	size_t head;
	size_t tail;
	size_t max_len;
};

/** @brief Ring buffer iterator structure
 */
 struct float_ring_buffer_iter {
	struct float_ring_buffer *buf;
	int idx;
};

/** @brief Resets ring buffer
 *
 * The functions sets state of a ring buffer to be empty.
 *
 * @param buf	pointer to ring buffer instance
 */
void ring_buffer_reset(struct float_ring_buffer *buf);

/** @brief Initializes ring buffer
 *
 * The functions should be called before first use of a ring buffer.
 * It is responsible for initialization of internals of an instance of ring
 * buffer.
 *
 * @param buf	pointer to ring buffer instance
 */
void ring_buffer_init(struct float_ring_buffer *buf);

/** @brief Returns information if ring buffer instance is full
 *
 * @param buf	pointer to ring buffer instance
 *
 * @retval		true if ring buffer is full
 * @retval		false if ring buffer has available storage
 */
bool ring_buffer_is_full(struct float_ring_buffer *buf);

/** @brief Returns information if there are no elements in ring buffer instance
 *
 * @param buf	pointer to ring buffer instance
 *
 * @retval		true if ring buffer is empty
 * @retval		false if ring buffer has one or more items availabe
 */
bool ring_buffer_is_empty(struct float_ring_buffer *buf);


/** @brief Returns maximum capacity of a ring buffer
 *
 * @param buf	pointer to ring buffer instance
 *
 * @returns		maximum number of items to be stored in ring buffer
 */
size_t ring_buffer_max_len(struct float_ring_buffer *buf);

/** @brief Returns number of items currently stored in ring buffer instance
 *
 * @param buf	pointer to ring buffer instance
 *
 * @returns		number of items stored in the ring buffer
 */
size_t ring_buffer_len(struct float_ring_buffer *buf);

/** @brief Pushes provided data to ring_buffe
 *
 * The function pushes the data to ring_buffer.
 * If buffer is full, the oldest not popped value will be overwritten.
 *
 * @param buf	pointer to ring buffer instance
 * @param data	value to be stored in the instance of ring buffer
 */
void ring_buffer_push(struct float_ring_buffer *buf, struct complex *data);

/** @brief Pops oldest available data
 *
 * The popped data may not be read again.
 * It is removed from instance of a ring buffer.
 * If instance is empty the function returns -ENODATA.
 *
 * @param buf	pointer to ring buffer instance
 * @param data	pointer where to store popped data
 *
 * @retval		zero if data popped successfully
 * @retval		-EINVAL if @p data or @p buf is a NULL pointer
 * @retval		-ENODATA if instance is empty
 */
int ring_buffer_pop(struct float_ring_buffer *buf, struct complex *data);

/** @brief Provides ring buffer iterator
 *
 * Provided iterator will point to oldest element stored in an instance
 * of a ring buffer.
 * If instance is empty it points to no element. To avoid of read of garbage
 * check if instance of a ring buffer has any element inside.
 *
 * @param buf	pointer to ring buffer instance
 * @param iter	pointer to iterator instance
 *
 * @retval		zero if iterator created successfully
 * @retval		-EINVAL if @p buf or @p iter is a NULL pointer
 */
int ring_buffer_get_iterator(struct float_ring_buffer *buf,
			     struct float_ring_buffer_iter *iter);

/** Iterators give a possibility to go through all elements stored in an instance
 * of a ring buffer without removing the items from instance memory.
 *
 * @note Pay attention that if you push or pop an element to an instance then
 * all iterators should be invalidated and re-created by call to ring_buffer_get_iterator.
 */

/** @brief Advance iterator to point to next element.
 *
 * @note The function does not check if end of element was reached.
 * 			 It means, the iterator will read past last valid element
 * 			 and loop back at the end of available storage.
 * 			 If instance is full, then it will loop back and start to read
 * 			 elements from tail of a buffer.
 *
 * @param iter	pointer to iterator instance
 *
 * @retval		zero if iterator advanced successfully
 * @retval		-EINVAL if @p iter is a NULL pointer
 */
int ring_buffer_iter_advance(struct float_ring_buffer_iter *iter);

/** @brief Resets iterator to point oldest item in a ring buffer.
 *
 * @param iter	pointer to iterator instance
 *
 * @retval		zero if iterator reset was successfull
 * @retval		-EINVAL if @p iter is a NULL pointer
 */
int ring_buffer_iter_reset(struct float_ring_buffer_iter *iter);

/** @brief Return information if iterator points to one past last element.
 *
 * @param iter	pointer to iterator instance
 *
 * @retval		true if iterator reached end of ring buffer elements
 * @retval		false if iterator did not reach end of ring buffer elements
 */
bool ring_buffer_iter_is_end(struct float_ring_buffer_iter *iter);

/** @brief Gets data pointed by iterator and advances iterator
 *
 * @param iter	pointer to iterator instance
 * @param data	pointer where to store data
 *
 * @retval		zero if iterator get operation finished successfully
 * @retval		-EINVAL if @p iter or @p is a NULL pointer
 */
int ring_buffer_iter_get(struct float_ring_buffer_iter *iter, struct complex *data);

/** @brief Gets data pointed by iterator
 *
 * @param iter	pointer to iterator instance
 * @param data	pointer where to store data
 *
 * @retval		zero if iterator get operation finished successfully
 * @retval		-EINVAL if @p iter or @p is a NULL pointer
 */
int ring_buffer_iter_read(struct float_ring_buffer_iter *iter, struct complex *data);

#endif /* AOA_LOCATOR_SRC_FLOAT_RING_BUFFER_H_ */
