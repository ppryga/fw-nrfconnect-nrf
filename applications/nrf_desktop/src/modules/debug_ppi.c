  
/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <hal/nrf_gpiote.h>
#include <nrfx_ppi.h>
#include <logging/log.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_rtc.h>
#include <hal/nrf_clock.h>

LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_DEBUG_PPI_LOG_LEVEL);

#define RTC COND_CODE_1(CONFIG_USE_RTC2, (NRF_RTC2), (NRF_RTC0))

const u32_t z_bt_ctlr_used_nrf_ppi_channels = 0;

static int gpiote_task_channel_allocate(u32_t pin)
{
	for(u8_t channel=0; channel < GPIOTE_CH_NUM; channel++) {
		if (!nrf_gpiote_te_is_enabled(channel)) {
				nrf_gpiote_task_configure(channel, pin,
										  NRF_GPIOTE_POLARITY_TOGGLE,
										  NRF_GPIOTE_INITIAL_VALUE_LOW);
				nrf_gpiote_task_enable(channel);
				return channel;
		}
	}
	return -1;
}

typedef enum {
	TASK_SET = 0,
	TASK_CLR,
	TASK_OUT,
	TASK_MAX
} TASK_TYPE;

static u32_t gpiote_get_task_addr(int gpiote_channel, TASK_TYPE type)
{
	u32_t task;
	nrf_gpiote_tasks_t task_id;
	
	switch(type)
	{
	case TASK_SET:
		task_id = offsetof(NRF_GPIOTE_Type,  TASKS_SET[gpiote_channel]);
	case TASK_CLR:
		task_id = offsetof(NRF_GPIOTE_Type,  TASKS_CLR[gpiote_channel]);
	case TASK_OUT:
	default:
		task_id = offsetof(NRF_GPIOTE_Type,  TASKS_OUT[gpiote_channel]);
	}
	task = nrf_gpiote_task_addr_get(task_id);

	return task;
}

static int ppi_trace_config(u32_t event, u32_t task, nrf_ppi_channel_t* new_channel)
{
    nrf_ppi_channel_t trigger_ch;
    int err;

    err = nrfx_ppi_channel_alloc(&trigger_ch);
    if (err != NRFX_SUCCESS) {
        LOG_ERR("Failed to allocate PPI channel");
		return err;
    }

	err = nrfx_ppi_channel_assign(trigger_ch, event, task);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to configure channel: %" PRIu8, trigger_ch);
		return err;
	}

	*new_channel = trigger_ch;
	return 0;
}

static void ppi_enable_channel(u32_t channel_mask)
{ 
    nrf_ppi_channels_enable(channel_mask);
}

static void ppi_disable_channel(u32_t channel_mask)
{
    nrf_ppi_channels_disable(channel_mask);
}

static int enable_radio_trace_event(u8_t gpiote_pin, u32_t event, TASK_TYPE task_t)
{
	u32_t gpiote_ch;
		
	gpiote_ch = gpiote_task_channel_allocate(gpiote_pin);
	if (gpiote_ch < 0) {
		LOG_ERR("Can't allocate gpio for ratio ready trace.");
		return gpiote_ch;
	}

	u32_t task = gpiote_get_task_addr(gpiote_ch, task_t);
	if (gpiote_ch < 0) {
		LOG_ERR("Can't allocate gpio for ratio ready trace.");
		return gpiote_ch;
	}
	
	int err;
	nrf_ppi_channel_t ppi_channel = NRF_PPI_CHANNEL0;

	err = ppi_trace_config(event, task, &ppi_channel);
	if (err) {
		LOG_ERR("Can't configure PPI for radio ready trace");
		return err;
	}
	
	ppi_enable_channel(BIT(ppi_channel));

	return 0;
}

void enable_radio_on_off_trace()
{
	u32_t start_evt;
	u32_t stop_evt;

	start_evt = nrf_radio_event_address_get(NRF_RADIO_EVENT_READY);
	stop_evt = nrf_radio_event_address_get(NRF_RADIO_EVENT_END);

	enable_radio_trace_event(3, start_evt, TASK_SET);
	enable_radio_trace_event(3, stop_evt, TASK_CLR);

	//nrf_rtc_event_enable(RTC, NRF_RTC_INT_TICK_MASK);
	//stop_evt = nrf_rtc_event_address_get(RTC, NRF_RTC_EVENT_TICK);
	stop_evt = nrf_clock_event_address_get(NRF_CLOCK_EVENT_LFCLKSTARTED);
	enable_radio_trace_event(28, stop_evt, TASK_OUT);
	LOG_INF("Radio trace enabled");
}