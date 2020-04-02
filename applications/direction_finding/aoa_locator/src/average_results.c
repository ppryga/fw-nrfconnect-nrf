#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "average_results.h"
#include "float_ring_buffer.h"

#ifndef M_PI
/** PI constant */
#define M_PI		3.14159265358979323846
#endif

/** Convert angle in degrees to complex number
 *
 * Converted angle is a unit vector represented in form of
 * real and imaginary part, because of that it is a complex
 * representation of angle.
 *
 * @param[in] angle	Angle value in degrees
 *
 * @retrun Returns structure that represents complex number.
 */
static struct complex angle_to_complex(float angle);

/** Convert complex number to angle in degrees
 *
 * Converts a unit vector represented in form of
 * real and imaginary part to angle in degrees.
 *
 * @param[in] vector	Complex number that represent angle
 *
 * @retrun Returns angle value in degrees.
 */
float complex_to_angle(struct complex vector);

/** Convert angle in degrees to angle in radians
 *
 * @param[in] angle_deg	Angle value in degrees
 *
 * @retrun Returns angle value in radians
 */
static inline float deg_to_radian(float angle_deg)
{
	return (M_PI / 180.0f) * angle_deg;
}

/** Convert angle in radians to angle in degrees
 *
 * @param[in] angle_rad	Angle value in radians
 *
 * @retrun Returns angle value in degrees
 */
static inline float radian_to_deg(float angle_rad)
{
	return (180.0f / M_PI) * angle_rad;
}

int average_results(const struct aoa_results *results, struct aoa_results* average)
{
	static struct float_ring_buffer azimuth_buffer;
	static struct float_ring_buffer elevation_buffer;

	static struct complex g_azimuth_sum;
	static struct complex g_elevation_sum;

	if (results == NULL || average == NULL) {
		return -EINVAL;
	}

	static bool init = false;

	if (init == false) {
		ring_buffer_init(&azimuth_buffer);
		ring_buffer_init(&elevation_buffer);

		g_azimuth_sum.real = 0.0;
		g_azimuth_sum.imag = 0.0;
		g_elevation_sum.real = 0.0;
		g_elevation_sum.imag = 0.0;

		init = true;
	}

	struct complex oldest_data = { 0.0, 0.0 };

	if( ring_buffer_is_full(&azimuth_buffer)) {
		ring_buffer_pop(&azimuth_buffer, &oldest_data);
	}

	struct complex angle = angle_to_complex(results->raw_result.azimuth);

	g_azimuth_sum.real = g_azimuth_sum.real - oldest_data.real + angle.real;
	g_azimuth_sum.imag = g_azimuth_sum.imag - oldest_data.imag + angle.imag;
	ring_buffer_push(&azimuth_buffer, &angle);
	size_t len = ring_buffer_len(&azimuth_buffer);

	float angle_rad;

	average->raw_result.azimuth = complex_to_angle(g_azimuth_sum);

	oldest_data.real = 0.0;
	oldest_data.imag = 0.0;
	if( ring_buffer_is_full(&elevation_buffer)) {
		ring_buffer_pop(&elevation_buffer, &oldest_data);
	}

	angle = angle_to_complex(results->raw_result.elevation);

	g_elevation_sum.real = g_elevation_sum.real - oldest_data.real + angle.real;
	g_elevation_sum.imag = g_elevation_sum.imag - oldest_data.imag + angle.imag;
	ring_buffer_push(&elevation_buffer, &angle);
	len = ring_buffer_len(&elevation_buffer);

	average->raw_result.elevation = complex_to_angle(g_elevation_sum);

	return 0;
}

int low_pass_filter_IIR(const struct aoa_results *results, struct aoa_results* filtered, float alpha)
{
	if (results == NULL || filtered == NULL) {
		return -EINVAL;
	}

	if (alpha < 0 || alpha > 1) {
		return -EINVAL;
	}

	static float azimuth_sum = 0.0;
	static float elevation_sum = 0.0;

	// LPF -> Y[n] = (1-a)*Y[n-1] + (a*current) = Y[n-1] - a*(Y[n-1] - current);
	azimuth_sum = azimuth_sum - (alpha * (azimuth_sum - results->raw_result.azimuth));
	elevation_sum = elevation_sum - (alpha * (elevation_sum - results->raw_result.elevation));

	filtered->raw_result.azimuth = azimuth_sum;
	filtered->raw_result.elevation = elevation_sum;

	return 0;
}

int low_pass_filter_FIR(const struct aoa_results *results, struct aoa_results* filtered)
{
	if (results == NULL || filtered == NULL) {
		return -EINVAL;
	}

	static struct float_ring_buffer azimuth_buffer;
	static struct float_ring_buffer elevation_buffer;

	static bool init = false;
	if (init == false) {
		ring_buffer_init(&azimuth_buffer);
		ring_buffer_init(&elevation_buffer);

		init = true;
	}

	struct complex angle;

	angle = angle_to_complex(results->raw_result.azimuth);
	ring_buffer_push(&azimuth_buffer, &angle);
	angle = angle_to_complex(results->raw_result.elevation);
	ring_buffer_push(&elevation_buffer, &angle);

	struct complex azimuth_filtered = { 0.0, 0.0 };
	struct complex elevation_filtered = { 0.0, 0.0 };

	size_t len = ring_buffer_len(&azimuth_buffer);
	float alpha = (len == 1) ? 1.0: 2.0/(float)len;
	float dt = alpha/(len+1);

	//for more than one sample do not start from 1.0;
//	if(dt != alpha) {
//		alpha -= dt;
//	}
	alpha = dt;

	struct float_ring_buffer_iter iter;

	ring_buffer_get_iterator(&azimuth_buffer, &iter);

	struct complex sample = {0.0, 0.0};
	while(ring_buffer_iter_is_end(&iter) == false){
		ring_buffer_iter_read(&iter, &sample);

		azimuth_filtered.real += (sample.real * alpha);
		azimuth_filtered.imag += (sample.imag * alpha);
		alpha += dt;

		ring_buffer_iter_advance(&iter);
	}

	ring_buffer_get_iterator(&elevation_buffer, &iter);

	alpha = dt;

	while(ring_buffer_iter_is_end(&iter) == false) {
		ring_buffer_iter_read(&iter, &sample);

		elevation_filtered.real += (sample.real * alpha);
		elevation_filtered.imag += (sample.imag * alpha);
		alpha += dt;
		ring_buffer_iter_advance(&iter);
	}

	filtered->raw_result.azimuth = complex_to_angle(azimuth_filtered);
	filtered->raw_result.elevation = complex_to_angle(elevation_filtered);

	return 0;
}

static struct complex angle_to_complex(float angle)
{
	assert( angle >= 0.0f && angle < 360.0f );

	float_t angle_rad = deg_to_radian(angle);

	struct complex complex_angle;

	complex_angle.real = cos(angle_rad);
	complex_angle.imag = sin(angle_rad);

	return complex_angle;
}

float complex_to_angle(const struct complex vector)
{
	float angle_rad;

	angle_rad = atan2f(vector.imag, vector.real);
	float angle_deg = radian_to_deg(angle_rad);

	angle_deg = (angle_deg >= 0.0f) ? angle_deg : (360.0f + angle_deg);
	return angle_deg;
}
