#ifndef SRC_AVERAGE_RESULTS_H_
#define SRC_AVERAGE_RESULTS_H_

#include "aoa.h"

int Average_results(const struct aoa_results *results, struct aoa_results* average);
int LowPassFilter(const struct aoa_results *results, struct aoa_results* filtered, float alpha);
int LowPassFilter_FIR(const struct aoa_results *results, struct aoa_results* filtered);

#endif /* SRC_AVERAGE_RESULTS_H_ */
