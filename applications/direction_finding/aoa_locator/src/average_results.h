#ifndef SRC_AVERAGE_RESULTS_H_
#define SRC_AVERAGE_RESULTS_H_

#include "aoa.h"

int average_results(const struct aoa_results *results, struct aoa_results* average);
int low_pass_filter_IIR(const struct aoa_results *results, struct aoa_results* filtered, float alpha);
int low_pass_filter_FIR(const struct aoa_results *results, struct aoa_results* filtered);

#endif /* SRC_AVERAGE_RESULTS_H_ */
