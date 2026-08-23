// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

#include "skif-arduino.h"

#define IN_0 1 // D15 -> PC1
#define IN_1 0 // D14 -> PC0
#define IN_PORT PORTC
#define IN_PIN PINC
#define IN_DDR DDRC

#define COUNTER_LIMIT 32

enum SysStatus {
	SysStop,
	SysStart,
	SysRunning,
};

volatile unsigned char Counter = 0;
volatile unsigned char MaxCounter = DEFAULT_MAX_COUNTER;
volatile unsigned char CurrSysStatus = SysStop;
volatile unsigned char CurrPinStatus = 0;
volatile unsigned char PrevPinStatus = 0;
volatile unsigned char TimerExpired = 0;
volatile unsigned char Rate = DEFAULT_RATE;
volatile unsigned char PinMaskCount = DEFAULT_DEBOUNCE_COUNTER;
volatile unsigned char PinMaskCounter0 = 0;
volatile unsigned char PinMaskCounter1 = 0;

static void serial_send(unsigned char);

static void update_pin_status(void)
{
	unsigned char d = IN_PIN;

	CurrPinStatus = 0;

	if (!(d & (1 << IN_0)))
		CurrPinStatus |= PIN0_ON;

	if (!(d & (1 << IN_1)))
		CurrPinStatus |= PIN1_ON;
}

static bool recv_one_char(unsigned char *c)
{
	unsigned char s;

	if ((s = UCSR0A) & 0x80) {
		*c = UDR0;
		return (s & 0x10) ? false : true;
	}

	return false;
}

static void serial_receive(void)
{
	unsigned char d;

	while (recv_one_char(&d)) {
		switch (d) {
		case CMD_RATE(0) ... CMD_RATE(7):
			Rate = d & 0x07;
			timer_init();
			timer_start();
			serial_send(0);
			CurrSysStatus = SysStop;
			break;
		case CMD_RESET:
			MaxCounter = DEFAULT_MAX_COUNTER;
			Rate = DEFAULT_RATE;
			PinMaskCount = DEFAULT_DEBOUNCE_COUNTER;
			serial_send(0);
			/* FALLTHROUGH */
		case CMD_STOP:
			CurrSysStatus = SysStop;
			break;
		case CMD_START:
			Counter = 0;
			CurrSysStatus = SysStart;
			break;
		case CMD_DEBOUNCE_COUNTER:
			while (!recv_one_char(&PinMaskCount));
			serial_send(0);
			CurrSysStatus = SysStop;
			break;
		case CMD_MAX_COUNTER:
			while (!recv_one_char(&d));
			d &= COUNTER_MASK;
			if (d) MaxCounter = d;
			serial_send(0);
			CurrSysStatus = SysStop;
			break;
		}
	}
}

ISR(TIMER1_COMPA_vect)
{
	update_pin_status();
	serial_receive();

	if (CurrSysStatus == SysRunning) {
		Counter++;

		if (PinMaskCounter0)
			PinMaskCounter0--;
		if (PinMaskCounter1)
			PinMaskCounter1--;
	}

	TimerExpired = 1;
}

static void input_init(void)
{
	/* set input pins */
	IN_DDR &= ~((1 << IN_0) | (1 << IN_1));
	MCUCR &= 0xef; // clear PUD (-> pull-up enable)
	IN_PORT |= ((1 << IN_0) | (1 << IN_1));
}

static void timer_init(void)
{
	TIMSK0 = 0;		// disble Arduino system timer (required)
	TCCR0B = 0;

	TIMSK1 = 0;
	TCCR1B = 0;
	TCCR1A = 0;
	TCCR1C = 0;
}

static void timer_start(void)
{
	OCR1A = (125 << Rate) - 1;	// 62.5us~8ms
	TCNT1 = 0;
	TIFR1 = ~0;
	TCCR1B = 0x0a;		// CTC, F_CLK / 8 (1tick = 500ns @ 16MHz)
	TIMSK1 = 0x02;		// Output compare A interrupt enable
}

static void serial_init(void)
{
	UBRR0H = 0x00;		// 500kbps @ 16MHz
	UBRR0L = 0x03;
	UCSR0A = 0x02;		// double-speed enabled
	UCSR0B = 0x18;		// interrupt disable, tx/rx enable
	UCSR0C = 0x06;		// async, non-parity, 1 stop-bit, 8 data-bit
}

static void serial_send(unsigned char d)
{
	while (!(UCSR0A & 0x20));	// wait for UDR ready
	UDR0 = d;
}

void setup(void)
{
	serial_init();
	input_init();
	timer_init();
	timer_start();
}

void loop(void)
{
	unsigned char changed, useprev;

	while (!TimerExpired)
	      asm __volatile__("sleep");

	cli();
	switch (CurrSysStatus) {
	default:
		/* do nothing */
		break;

	case SysStart:
		/* counter is 0 at this point */
		serial_send(CurrPinStatus);
		PrevPinStatus = CurrPinStatus;
		CurrSysStatus = SysRunning;
		PinMaskCounter0 = PinMaskCounter1 = 0;
		break;

	case SysRunning:
		changed = (CurrPinStatus ^ PrevPinStatus) & PIN_MASK;
		useprev = ((PinMaskCounter0 ? PIN0_ON : 0) |
			   (PinMaskCounter1 ? PIN1_ON : 0));

		if ((changed & ~useprev) || Counter >= MaxCounter) {
			serial_send(PrevPinStatus | Counter);
			PrevPinStatus &= useprev;
			PrevPinStatus |= (CurrPinStatus & ~useprev);
			Counter = 0;
			if (changed & ~useprev & PIN0_ON)
				PinMaskCounter0 = PinMaskCount;
			if (changed & ~useprev & PIN1_ON)
				PinMaskCounter1 = PinMaskCount;
		}
		break;
	}
	TimerExpired = 0;
	sei();
}
