#ifndef SRC_AVERAGE_RESULTS_H_
#define SRC_AVERAGE_RESULTS_H_

#include "aoa.h"

int Average_results(const aoa_results *results, aoa_results* average);
int LowPassFilter(const aoa_results *results, aoa_results* filtered, float alpha);
int LowPassFilter_FIR(const aoa_results *results, aoa_results* filtered);

#endif /* SRC_AVERAGE_RESULTS_H_ */
