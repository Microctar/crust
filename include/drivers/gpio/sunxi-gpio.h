/*
 * Copyright © 2017-2019 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#ifndef DRIVERS_GPIO_SUNXI_GPIO_H
#define DRIVERS_GPIO_SUNXI_GPIO_H

#include <gpio.h>

#define SUNXI_GPIO_PIN(port, index) (32 * (port) + (index))

extern struct device r_pio;

#endif /* DRIVERS_GPIO_SUNXI_GPIO_H */
