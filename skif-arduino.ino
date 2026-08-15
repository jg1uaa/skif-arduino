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
volatile unsigned char CurrSysStatus = SysStop;
volatile unsigned char CurrPinStatus = 0;
volatile unsigned char PrevPinStatus = 0;
volatile unsigned char TimerExpired = 0;
volatile unsigned char Rate = 1;

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

static void serial_receive(void)
{
	unsigned char d, s;

	while ((s = UCSR0A) & 0x80) {
		d = UDR0;

		/* framing error */
		if (s & 0x10)
			continue;

		switch (d) {
		case CMD_RATE(0) ... CMD_RATE(7):
			Rate = d & 0x07;
			timer_init();
			timer_start();
			/* FALLTHROUGH */
		case CMD_READY:
			serial_send(d);
			/* FALLTHROUGH */
		case CMD_STOP:
			CurrSysStatus = SysStop;
			break;
		case CMD_START:
			Counter = 0;
			CurrSysStatus = SysStart;
			break;
		case CMD_QUERY_RATE:
			serial_send(CMD_RATE(Rate));
			break;
		}
	}
}

ISR(TIMER1_COMPA_vect)
{
	update_pin_status();
	serial_receive();

	if (CurrSysStatus == SysRunning)
		Counter++;

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
		break;

	case SysRunning:
		if (((CurrPinStatus ^ PrevPinStatus) & PIN_MASK) ||
		    Counter >= COUNTER_LIMIT) {
			serial_send(PrevPinStatus | Counter);
			PrevPinStatus = CurrPinStatus;
			Counter = 0;
		}
		break;
	}
	TimerExpired = 0;
	sei();
}
