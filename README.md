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

Note: Debouncing is not defined in this protocol. Users can implement software logic and/or hardware circuits.


## Commands

### CMD_START ('-')

Start capturing. Capturing will be stopped after receiving CMD_STOP, CMD_READY and CMD_RATE command.

### CMD_STOP ('.')

Simply stops capturing without response.

### CMD_READY ('/')

Stops capturing and send '/' response.

Applications should use this command at startup to check controller is connected and ready.
This command does not affect the sampling rate setting. If needed, applications issue CMD_RATE command.

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

Returns new or current (if failed) mode as response.

### CMD_QUERY_RATE (',')

Returns current sampling rate setting with '0' ... '7'.


## Data format

|Bit 7|Bit 6|Bit 5:0|
|----|----|----|
|PIN1|PIN0|Timestamp|

- PIN0: PIN0 status (0 = key off, 1 = key on)
- PIN1: PIN1 status (0 = key off, 1 = key on)
- Timestamp: Elapsed time (unit: clock ticks defined at CMD_RATE). The value will be between 0 to 32.


## License

GPL v3.0 or later

