/*
 * Copyright © 2017-2019 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#include <css.h>
#include <device.h>
#include <irq.h>
#include <pmic.h>
#include <scpi.h>
#include <stdbool.h>
#include <system.h>
#include <util.h>
#include <watchdog.h>
#include <watchdog/sunxi-twd.h>

static uint8_t system_state;

bool
system_can_wake(void)
{
	return system_state == SYSTEM_INACTIVE ||
	       system_state == SYSTEM_OFF;
}

bool
system_is_running(void)
{
	return system_state == SYSTEM_ACTIVE;
}

void
system_state_init(void)
{
	uint8_t online = css_get_online_cores(0);

	/* If all cores are off, assume the system is off. */
	if (online == 0) {
		system_state = SYSTEM_OFF;
		return;
	}

	/* Initialize runtime services. */
	scpi_init();

	/* If secondary cores are on, Linux is already running. */
	if (online != BIT(0))
		return;

	/* Inform SCPI clients that the firmware has finished booting. */
	scpi_create_message(SCPI_CLIENT_EL3, SCPI_CMD_SCP_READY);
}

void
system_state_machine(void)
{
	const struct device *pmic, *watchdog;

	switch (system_state) {
	case SYSTEM_ACTIVE:
		/* Poll runtime services. */
		scpi_poll();

		break;
	case SYSTEM_SUSPEND:
		/* Disable runtime services. */
		scpi_exit();

		/* Enable wakeup sources. */

		/* Perform PMIC-specific suspend actions. */
		if ((pmic = pmic_get())) {
			pmic_suspend(pmic);
			device_put(pmic);
		}

		/* Turn off all unnecessary power domains. */

		/* Turn off all unnecessary clocks. */

		/* The system is now inactive. */
		system_state = SYSTEM_INACTIVE;
		break;
	case SYSTEM_INACTIVE:
		/* Poll wakeup sources. Resume on wakeup. */
		if (irq_poll())
			system_state = SYSTEM_RESUME;
		break;
	case SYSTEM_RESUME:
		/* Turn on previously-disabled clocks. */

		/* Turn on previously-disabled power domains. */

		/* Perform PMIC-specific resume actions. */
		if ((pmic = pmic_get())) {
			pmic_resume(pmic);
			device_put(pmic);
		}

		/* Disable wakeup sources. */

		/* Enable runtime services. */
		scpi_init();

		/* Resume execution on the first CPU in the CSS. */
		css_set_css_state(SCPI_CSS_ON);
		css_set_cluster_state(0, SCPI_CSS_ON);
		css_set_core_state(0, 0, SCPI_CSS_ON);

		/* The system is now active. */
		system_state = SYSTEM_ACTIVE;
		break;
	case SYSTEM_SHUTDOWN:
		/* Disable runtime services. */
		scpi_exit();

		/* Enable a subset of wakeup sources. */

		/* Perform PMIC-specific shutdown actions. */
		if ((pmic = pmic_get())) {
			pmic_shutdown(pmic);
			device_put(pmic);
		}

		/* Turn off all possible power domains. */

		/* Turn off all possible clocks. */

		/* The system is now off. */
		system_state = SYSTEM_OFF;
		break;
	case SYSTEM_OFF:
		/* Poll wakeup sources. Reset on wakeup. */
		if (!irq_poll())
			system_state = SYSTEM_RESET;
		break;
	case SYSTEM_RESET:
		/* Attempt to reset the SoC using the PMIC. */
		if ((pmic = pmic_get())) {
			pmic_reset(pmic);
			device_put(pmic);
		}

		/* Attempt to reset the SoC using the watchdog. */
		if ((watchdog = device_get(&r_twd.dev))) {
			watchdog_enable(watchdog, 1);
			device_put(watchdog);
		}

		/* Leave the system state as is; continue making reset
		 * attempts each time this function is called. */
		break;
	}
}

void
system_reset(void)
{
	system_state = SYSTEM_RESET;
}

void
system_shutdown(void)
{
	if (system_state == SYSTEM_ACTIVE)
		system_state = SYSTEM_SHUTDOWN;
}

void
system_suspend(void)
{
	if (system_state == SYSTEM_ACTIVE)
		system_state = SYSTEM_SUSPEND;
}

void
system_wakeup(void)
{
	if (system_state == SYSTEM_INACTIVE)
		system_state = SYSTEM_RESUME;
	if (system_state == SYSTEM_OFF)
		system_state = SYSTEM_RESET;
}