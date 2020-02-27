#include <assert.h>
#include <string.h>

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <bluetooth/dfe_data.h>

#include "if.h"
#include "protocol.h"
#include "dfe_local_config.h"
#include "ble.h"

extern struct k_msgq df_packet_msgq;

void main(void)
{
	printk("Starting AoA Locator CL!\r\n");
	/* initialize UART interface to provide I/Q samples */
	struct if_data* iface = IF_Initialization();

	if (iface == NULL) {
		printk("Locator stopped!\r\n");
		return;
	}

	int err;

	err = PROTOCOL_Initialization(iface);
	if (err) {
		printk("Locator stopped!\r\n");
		return;
	}

	const struct df_sampling_config* sampl_conf = NULL;
	const struct df_antenna_config* ant_conf = NULL;
	const struct dfe_ant_gpio* ant_gpio = NULL;
	u8_t ant_gpio_len = 0;

	sampl_conf = df_get_sampling_config();
	ant_conf = df_get_antenna_config();
	dfe_get_ant_gpios_config(&ant_gpio, &ant_gpio_len);

	assert(sampl_conf != NULL);
	assert(ant_conf != NULL);
	assert(ant_gpio != NULL);
	assert(ant_gpio_len != 0);

	printk("Initialize DFE\r\n");
	err = dfe_init(sampl_conf, ant_conf, ant_gpio, ant_gpio_len);
	if (err) {
		printk("Locator stopped!\r\n");
		return;
	}

	printk("Initialize Bluetooth\r\n");
	BLE_Initialization();

	k_sleep(K_MSEC(100));
	while(1)
	{
		static bool no_data = false;
		static struct df_packet df_data_packet;
		static struct dfe_mapped_packet df_data_mapped;

		memset(&df_data_packet, 0, sizeof(df_data_packet));
		memset(&df_data_mapped, 0, sizeof(df_data_mapped));
		k_msgq_get(&df_packet_msgq, &df_data_packet, K_NO_WAIT);
		if (df_data_packet.hdr.length != 0) {
			no_data = false;
			printk("\r\nData arrived...\r\n");

			df_map_iq_samples_to_antennas(&df_data_mapped,
						      &df_data_packet,
						      sampl_conf, ant_conf);
			err = PROTOCOL_Handling(sampl_conf, &df_data_mapped);
			if (err) {
				printk("Error in protocol handling!\r\n");
				printk("Locator stopped!\r\n");
				return;
			}
		}
		else
		{
			if(!no_data) {
				printk("\r\nNo data received.");
				no_data = true;
			} else {
				printk(".");
			}
		}
	 	k_sleep(K_MSEC(1));
	}
}
