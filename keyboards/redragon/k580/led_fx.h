/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * Lighting effects for two zones with separate settings: the key LEDs and the side lights.
 * Settings of a zone, in this order; an action is zone * LED_FX_SETTINGS + setting, with zone 0 =
 * keys and zone 1 = side lights.
 */
enum { LED_FX_MODE, LED_FX_HUE, LED_FX_SAT, LED_FX_VAL, LED_FX_SPEED, LED_FX_ON, LED_FX_SETTINGS };

/* Saved form of the settings: LED_FX_SAVE_SIZE bytes, zone 0 then zone 1, each in the order above. */
#define LED_FX_SAVE_SIZE (2 * LED_FX_SETTINGS)
/* Sets the settings from their saved form (in code space); a value out of range keeps its default. */
void led_fx_load(__code const uint8_t *p);
/* Writes the saved form of the settings to dst. */
void led_fx_save(__xdata uint8_t *dst);
/* Next step of a setting (previous step with reverse); LED_FX_ON toggles. */
void led_fx_adjust(uint8_t action, bool reverse);
/* Call from the main loop: renders the effects into the LED buffer, one COM per call. */
void led_fx_task(void);
