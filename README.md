# Simple Key Interface (SKIF)

---
## Introduction

There are many ways to connect a telegraph (morse) key to a PC. Modern PCs do not have an RS-232C port with a real UART (16550) controller. Therefore, users connect keys to the DTR/CTS/DTR lines via USB-UART adapters, or use USB-HID devices instead.

However, USB devices make it impossible to avoid the latency introduced by the USB host controller and protocol stack. A low-speed (1.5Mbps) USB-HID device uses interrupt transfer with 10ms interval, even if using Full-speed (12Mbps) it will be 1ms. Many USB-UART devices have 1ms latency because they use Full-speed bulk transfer.

This project is an experiment to monitor key status and timing using a microcontroller, ensuring accurate time measurement for key switching.


## Prerequisites

- Arduino UNO or compatible board, with USB-UART interface
- PC's USB-UART driver supports 500000bps, 8bit data, 1 stop bit and non-parity
- Your favorite straight key or paddle (supports up to 2-channel inputs)


## Commands

### CMD_START ('-')

Starts capturing. Captured data stream will be come without response. The first captured data is initial pin status with timestamp = 0.

### CMD_STOP ('.')

Simply stops capturing without response.

### CMD_RESET ('/')

Stops capturing, sets default state and send 0x00 response.

Applications should use this command at startup to check controller is connected and ready.

### CMD_RATE ('0' ... '7')

Stops capturing and sets new sampling rate.

- 0: 16k samples/sec (62.5us period)
- 1: 8k samples/sec (125us) [default]
- 2: 4k samples/sec (250us)
- 3: 2k samples/sec (500us)
- 4: 1k samples/sec (1ms)
- 5: 500 samples/sec (2ms)
- 6: 250 samples/sec (4ms)
- 7: 125 samples/sec (8ms)

Returns 0x00 as response.

### CMD_DEBOUNCE_COUNTER ('+')

Stops capturing and sets debounce counter. Following unsigned byte is counter value, 0x00 - 0xff. Default is 0x00 (debounce disabled). Returns 0x00 as response.

### CMD_MAX_COUNT ('*')

Stops capturing and sets max counter ticks. Following unsigned byte is counter value, 0x01 - 0x3f. Default is 0x20. Returns 0x00 as response.


## Data format

|Bit 7|Bit 6|Bit 5:0|
|----|----|----|
|PIN1|PIN0|Timestamp|

- PIN0: PIN0 status (0 = key off, 1 = key on)
- PIN1: PIN1 status (0 = key off, 1 = key on)
- Timestamp: Elapsed time (unit: clock ticks defined at CMD_RATE). The value will be between 0 to 32.


## License

GPL v3.0 or later

