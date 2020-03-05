/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <errno.h>
#include <device.h>
#include <uart.h>

#include "if.h"

static struct if_data g_if;
static void if_uart_app_isr(struct device *dev);
static void uart_send(uint8_t *buffer, uint16_t length);

struct if_data *if_initialization(void)
{
	g_if.uart_app.dev = device_get_binding(CONFIG_AOA_LOCATOR_UART_PORT);
	if (!g_if.uart_app.dev) {
		printk("[UART] - Device not found or cannot be used\r\n");
		return NULL;
	}

	uart_irq_rx_disable(g_if.uart_app.dev);
	uart_irq_tx_disable(g_if.uart_app.dev);
	uart_irq_callback_set(g_if.uart_app.dev, if_uart_app_isr);
	uart_irq_rx_enable(g_if.uart_app.dev);

	g_if.uart_app.send = uart_send;

	return &g_if;
}

static void if_uart_app_isr(struct device *dev)
{
	struct device_vector *uart = &g_if.uart_app;
	uint16_t i = 0;
	uint16_t index = 0;
	char buffer[255] = {0};

	while (uart_irq_update(dev) && uart_irq_is_pending(dev))
	{
		if (!uart_irq_rx_ready(dev))
		{
			if (uart_irq_tx_ready(dev))
			{
				printk("[UART] - transmit ready");
			}
			else
			{
				printk("[UART] - spurious interrupt");
			}

			break;
		}

		index = uart_fifo_read(dev, buffer, sizeof(buffer)-1);

		for(i=0; i<index; i++)
		{
			uart->rx_buffer[uart->rx_index] = buffer[i];

			if (++uart->rx_index == sizeof(uart->rx_buffer))
			{
				uart->rx_index = 0;
			}
		}
	}
}

static void uart_send(uint8_t *buffer, uint16_t length)
{
	struct device_vector *uart = &g_if.uart_app;

	size_t i = 0;

	for(i=0; i<length; i++)
	{
		uart_poll_out(uart->dev, buffer[i]);
	}
}
