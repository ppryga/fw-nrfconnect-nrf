/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_DESKTOP_DEBUG_GPIO_PINS_ENABLE)
#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define DEBUG_PORT       NRF_P1
#define DEBUG_PIN0       BIT(1)
#define DEBUG_PIN1       BIT(2)
#define DEBUG_PIN2       BIT(3)
#define DEBUG_PIN3       BIT(4)
#define DEBUG_PIN4       BIT(5)
/*#define DEBUG_PIN5       BIT(6)
#define DEBUG_PIN6       BIT(7)
#define DEBUG_PIN7       BIT(8)
#define DEBUG_PIN8       BIT(10)
#define DEBUG_PIN9       BIT(11)*/
#else
#error GPIO_DEBUG_PINS not supported on this board.
#endif

#define DEBUG_PIN_MASK   (DEBUG_PIN0 | DEBUG_PIN1 | DEBUG_PIN2 | DEBUG_PIN3 | \
		DEBUG_PIN4)

#define DEBUG_INIT() \
	do { \
		DEBUG_PORT->DIRSET = DEBUG_PIN_MASK; \
		DEBUG_PORT->OUTCLR = DEBUG_PIN_MASK; \
	} while (0)

#define DEBUG_CHANGE_OUTPUT(flag, pin) \
	do { \
		if (flag) { \
			DEBUG_PORT->OUTSET = (pin); \
			DEBUG_PORT->OUTCLR = (pin); \
		} else { \
			DEBUG_PORT->OUTCLR = (pin); \
			DEBUG_PORT->OUTSET = (pin); \
		} \
	} while (0)
#else
#error "xxx"
#define DEBUG_INIT()
#define DEBUG_CHANGE_OUTPUT(flag, pin)
#endif 