# M433_analyzer

This project is a 433Mhz transmission analysis tool for STM32F4Discovery (and
similar boards). Its role it to decode and interpret sentences captured from
a 433MHz receiver and display them using different mechanisms.

## Pre-requisites

Hardware:
* An STM32F4-Discovery board
* A 433MHz receiver like this one:
  http://www.ebay.com/itm/1-Set-433Mhz-RF-Transmitter-Receiver-Module-for-Raspberry-Arduino-ARM-MCU-/181308047424?pt=LH_DefaultDomain_0&hash=item2a36cd4040)
* An output module (either a UART module, or an ESP8266 wifi module)

Software:
* Keil MDK-ARM v5 (the demo version will work)
* The MDK5 software pack "STM32F4 Series Device Support, Drivers and Examples"
  which may be downloaded from http://www.keil.com/dd2/pack/

## How it works

### Decoders and filters

A decoder is a software module which takes as input an array of pulse durations, and
tries to decode and interpret the signal as human-readable information.

Each decoder provides requirements on the expected sentence:
* Min and max length of each pulse
* Minimum number of pulses in a sentence

Then, when a decoder is registered, a global filter is built, which defines the
requirements for a sentence to be recorded and treated by the decoders. If a suite
of pulses doesn't validate this filter, then it's considered as noise and discarded.

### Main module

The main loop actually does nothing: the program is interrupt-driven and updates its
status as soon as the state of the RF receiver changes.

If a pulse matches the global filter, it is added to the sentence being recorded.
Otherwise, if the sentence is long enough, every decoder is called to try and parse
it. The recording is then reset.

## Output modules

At the moment, the output data is printed on a UART (which may be connected to a
computer using a FTDI232-based module, or to a raspberry pi using dupont wires).

You may also connect an ESP8266 wifi module and transmit the data using UDP syslog
events (needs testing).

Another idea would be to store the captured senteces on a SD card (needs implementing).

## Usage

1. Download this project and edit the file `defines.h`. It describes all the GPIOs
and modules used by the project.
2. Compile the software and flash it to the board.
3. Enjoy!
