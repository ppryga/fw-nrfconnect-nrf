#ifndef SRC_AVERAGE_RESULTS_H_
#define SRC_AVERAGE_RESULTS_H_

#include "aoa.h"

/** @brief Evaluates average value of azimuth and elevation angles
 *
 * The function evaluates average values of azimuth and elevation angles
 * form current value and number of historical data stored in ring buffer.
 * Number of historical items stored in ring buffer is equivalent to max
 * ring buffer capacity @see FR_INTERNAL_BUFFER_SIZE.
 *
 * Current result is stored in the ring buffer to be used in a next call
 * as historical data.
 *
 * @param[in] results	current results
 * @param[out] average	averaged results
 *
 * @retval		zero if average evaluated successfully
 * @retval		-EINVAL if @p results or @p average is a NULL pointer
 */
int average_results(const struct aoa_results *results, struct aoa_results* average);

/** @brief Does IIR filtration of azimuth and elevation angles
 *
 * The function uses IIR filter to provide smoothened values of azimuth and
 * elevation angles. The filtered values are evaluated with following formula:
 * [n] = (1-a)*Y[n-1] + (a*current) = Y[n-1] - a*(Y[n-1] - current)
 *
 * @param[in]	results		current results
 * @param[out]	filtered	filtered results
 * @param[in]	alpha		coefficient used to evaluate how much historical data affect
 * 							final result
 *
 * @retval		zero if average evaluated successfully
 * @retval		-EINVAL if @p results or @p average is a NULL pointer
 */
int low_pass_filter_IIR(const struct aoa_results *results, struct aoa_results* filtered, float alpha);

/** @brief Does FIR filtration of azimuth and elevation angles
 *
 * The function uses FIR filter to provide smoothened values of azimuth and
 * elevation angles. It uses current value and number of historical data
 * stored in internal ring buffer. Number of historical items stored
 * in the ring buffer is equivalent to max ring buffer capacity
 * (@see BUFFER_SIZE in float_ring_buffer.h file).
 *
 * Current result is stored in the ring buffer to be used in a next call
 * as historical data.
 *
 * @param[in]	results		current results
 * @param[out]	filtered	filtered results
 *
 * @retval		zero if average evaluated successfully
 * @retval		-EINVAL if @p results or @p average is a NULL pointer
 */
int low_pass_filter_FIR(const struct aoa_results *results, struct aoa_results* filtered);

#endif /* SRC_AVERAGE_RESULTS_H_ */
