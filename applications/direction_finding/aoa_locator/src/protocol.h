/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <zephyr/types.h>
#include <bluetooth/dfe_data.h>
#include "dfe_local_config.h"
#include "if.h"
#include "aoa.h"

/** @brief  Header added to data message send via UART
 */
#define PROTOCOL_HEAD				"+AoA"
/** @brief Length of a string buffer used to send data via UART
 */
#define PROTOCOL_STRING_BUFFER_SIZE		10240

/** @brief UARD data transmission structure
 */
struct protocol_data
{
	/** @brief member to access UARD port
	 */
	struct if_data *uart;
	/** @brief transmission data buffer
	 */
	char string_packet[PROTOCOL_STRING_BUFFER_SIZE];
	/** @brief number of bytes stored in transmission buffer
	 */
	size_t stored_data_len;
};

/** @brief Initializes data transfer internals
 *
 * @param iface pointer to interface data
 *
 * @retval 0 successful initialization
 * @retval -EINVAL if @p iface is NULL
 */
int data_transfer_init(struct if_data *iface);

/** @brief Prepares data transfer
 *
 * The function should be used at the beginning of transfer transaction.
 * It resets number of bytes stored in transmission buffer.
 * Also it puts data package header into transfer buffer.
 *
 * @note After first transfer init function call the number of bytes
 * stored in transfer buffer will never be zero.
 * That is caused by placement message header into the buffer during init.
 */
void data_tranfer_prepare_header();

/** @brief Puts mapped IQ samples into transfer buffer
 *
 * The function stores IQ samples including information like: mapped antenna
 * index, time delay from the beginning of CTE reception (first sample).
 * Pay attention that antenna index 255 means sample taken during switch period.
 * Time data related with particular samples is an integer value.
 * The unit of the value is 125[us]. E.g. Time 4 means 4*125=500[us].
 *
 * @param smapl_conf	pointer to sampling configuration
 * @param mapped_data	pointer to IQ samples mapped to antennas
 */
void data_transfer_prepare_samples(const struct dfe_sampling_config *sampl_conf,
				  const struct dfe_mapped_packet *mapped_data);
/** @brief Puts angle of arrival results in transfer buffer
 *
 * @param smapl_conf	pointer to sampling configuration
 * @param results		pointer to result to be stored in transfer buffer
 */
void data_tranfer_prepare_results(const struct dfe_sampling_config *sampl_conf,
				const struct aoa_results *result);

/** @brief Prepares end of data transfer
 *
 * The function stores end of data footer in transfer buffer.
 * No data should be added to transfer buffer after this function call.
 */
void data_tranfer_prepare_footer();

/** @brief Sends data via UART
 */
void data_tranfer_send();

#endif
