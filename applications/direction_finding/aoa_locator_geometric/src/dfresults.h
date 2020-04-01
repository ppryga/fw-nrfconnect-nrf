/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __DFRESULTS_H
#define __DFRESULTS_H
#include <zephyr/types.h>

/**
 * @brief Calculation results
 *
 * The results of the angle calculation.
 * This structure is used to print out the data.
 */
struct dfresults {
	/**
	 * @brief The phase difference between antennas
	 */
	float phase;
	/**
	 * @brief Calculated aziumth angle in radians.
	 */
	float azimuth;
};

#endif /* __DFRESULTS_H */
