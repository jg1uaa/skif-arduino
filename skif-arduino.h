// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#ifndef SENSE_GPIO_ARDUINO_H
#define SENSE_GPIO_ARDUINO_H

#define PIN0_ON 0x40
#define PIN0_OFF 0x00
#define PIN1_ON 0x80
#define PIN1_OFF 0x00
#define PIN_MASK (PIN0_ON | PIN1_ON)
#define COUNTER_MASK (0xff ^ PIN_MASK)

#define CMD_RATE(x) ('0' + (x))
#define CMD_RESET '/'
#define CMD_STOP '.'
#define CMD_START '-'
#define CMD_QUERY_RATE ','
#define CMD_DEBOUNCE_COUNTER '+'
#define CMD_MAX_COUNTER '*'

#define DEFAULT_RATE 1
#define DEFAULT_DEBOUNCE_COUNTER 0
#define DEFAULT_MAX_COUNTER 32

#endif
