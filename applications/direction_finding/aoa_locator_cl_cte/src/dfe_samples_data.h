/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SRC_DFE_SAMPLES_DATA_H_
#define SRC_DFE_SAMPLES_DATA_H_

#include <stdint.h>

struct dfe_header {
	uint32_t length;
	uint32_t frequency;
} __attribute__((packed));

union dfe_iq_f {
	struct {
		float i; //12bit
		float q; //12bit
	};
	float data[2];
} __attribute__((packed));

/* Max length of SWITH PERIOD supported by our radio is 8us.
 * Smallest samples spacing is 125ns.
 * Max total number of gathered samples is 32 (that includes only sampling slots)
 */
#define DF_SAMPLES_COUNT (32)

struct dfe_samples {
	union dfe_iq_f data[DF_SAMPLES_COUNT];
	uint8_t samples_num;
	uint8_t antenna_id;
} __attribute__((packed));

/* This is a fixed value for reference period duration 8us and min sampling
 * spacing 125ns.
 */
#define DF_REF_SAMPLES_COUNT (64)

struct dfe_ref_samples {
	union dfe_iq_f data[DF_REF_SAMPLES_COUNT];
	uint8_t samples_num;
	uint8_t antenna_id;
} __attribute__((packed));


/* Max length of CTE is 160us. With removed guard period(4us) and reference period(8us)
 * we have 148us. Smallest reasonable antenna switching period is 2us.
 * That gives us up to 74 samples set per antenna.
 */
#define DF_MAX_EFFECTIVE_ANT_COUNT (32)
struct dfe_mapped_packet {
	struct dfe_header header;
	struct dfe_ref_samples ref_data;
	struct dfe_samples sampl_data[DF_MAX_EFFECTIVE_ANT_COUNT];
} __attribute__((packed));

#endif /* SRC_DFE_SAMPLES_DATA_H_ */
