#include <errno.h>
#include <stdbool.h>

#include "average_results.h"
#include "float_ring_buffer.h"

int Average_results(const aoa_results *results, aoa_results* average)
{
	static float_ring_buffer azimuth_buffer;
	static float_ring_buffer elevation_buffer;

	static float g_azimuth_sum;
	static float g_elevation_sum;

	if (results == NULL || average == NULL) {
		return -EINVAL;
	}

	static bool init = false;
	if (init == false) {
		ring_buffer_init(&azimuth_buffer);
		ring_buffer_init(&elevation_buffer);

		g_azimuth_sum = 0.0;
		g_elevation_sum = 0.0;

		init = true;
	}

	float oldest_data = 0.0;
	if( ring_buffer_is_full(&azimuth_buffer)) {
		ring_buffer_pop(&azimuth_buffer, &oldest_data);
	}

	g_azimuth_sum = g_azimuth_sum - oldest_data + results->raw_result.azimuth;
	ring_buffer_push(&azimuth_buffer, results->raw_result.azimuth);
	size_t len = ring_buffer_len(&azimuth_buffer);

	average->raw_result.azimuth = g_azimuth_sum/len;

	oldest_data = 0.0;
	if( ring_buffer_is_full(&elevation_buffer)) {
		ring_buffer_pop(&elevation_buffer, &oldest_data);
	}

	g_elevation_sum = g_elevation_sum - oldest_data + results->raw_result.elevation;
	ring_buffer_push(&elevation_buffer, results->raw_result.elevation);
	len = ring_buffer_len(&elevation_buffer);

	average->raw_result.elevation = g_elevation_sum/len;

	return 0;
}

int LowPassFilter_IIR(const aoa_results *results, aoa_results* filtered, float alpha)
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

int LowPassFilter_FIR(const aoa_results *results, aoa_results* filtered)
{
	if (results == NULL || filtered == NULL) {
		return -EINVAL;
	}

	static float_ring_buffer azimuth_buffer;
	static float_ring_buffer elevation_buffer;

	static bool init = false;
	if (init == false) {
		ring_buffer_init(&azimuth_buffer);
		ring_buffer_init(&elevation_buffer);

		init = true;
	}

	ring_buffer_push(&azimuth_buffer, results->raw_result.azimuth);
	ring_buffer_push(&elevation_buffer, results->raw_result.elevation);

	float azimuth_filtered = 0.0;
	float elevation_filtered = 0.0;


	size_t len = ring_buffer_len(&azimuth_buffer);
	float alpha = (len == 1) ? 1.0: 2.0/(float)len;
	float dt = alpha/(len+1);

	//for more than one sample do not start from 1.0;
//	if(dt != alpha) {
//		alpha -= dt;
//	}
	alpha = dt;

	float_ring_buffer_iter iter;

	ring_buffer_get_iterator(&azimuth_buffer, &iter);

	float sample = 0.0;
	while(ring_buffer_iter_is_end(&iter) == false){
		ring_buffer_iter_read(&iter, &sample);

		azimuth_filtered += (sample * alpha);
		alpha += dt;

		ring_buffer_iter_advance(&iter);
	}

	ring_buffer_get_iterator(&elevation_buffer, &iter);

	float test = 0.0;
	alpha = dt;

	while(ring_buffer_iter_is_end(&iter) == false) {
		ring_buffer_iter_read(&iter, &sample);

		elevation_filtered += (sample * alpha);
		alpha += dt;
		ring_buffer_iter_advance(&iter);
	}

	filtered->raw_result.azimuth = azimuth_filtered;
	filtered->raw_result.elevation = elevation_filtered;
	filtered->filtered_result.azimuth = test;

	return 0;
}
