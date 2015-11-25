# Examples

These examples provide a starting point and template on coding and documentation
for SmartUniversity student projects.

## Hardware

All examples were developed and tested using the _Atmel SAM R21 Xplained Pro_
board, see also:

* [Atmel website](http://www.atmel.com/tools/ATSAMR21-XPRO.aspx)
* [RIOT-OS wiki](https://github.com/RIOT-OS/RIOT/wiki/Board%3A-Samr21-xpro)

No further hardware components are required to run any of the following software
examples.

## Software

We provide the following distinct examples showing howto to interact with the
board and also send simple udp messages between 2 devices. Below you also find
a brief description to build the examples.

### src/button

This tool shows some basic interaction with the Atmel SAM R21 board, namely
control the onboard LED0 by pressing a button (SW0). Corresponding output
is also available via terminal connection.

### src/pingpong

This program demonstrates a simple network protocol. Node A sends a PING message
to the multicast group address ```[ff02::1]```, if a node B receives the PING it
response with a PONG message send to the link local unicast address of node A.
PING message can be initiated via terminal connection to access the shell. PONG
messages are send automatically when a PING was received.

Note: the source code is _universal_ meaning it compiles under Linux or MacOS X
as well. Just run ```gcc -o pingpong -lpthread main.c```.

### build and run

Download or clone this example repository to your development system. To build
the small tools you also need to download the
[RIOT](https://github.com/riot-os/riot) source code.

Set the following environmental variables according to you system:
```
export BOARD=samr21-xpro
export PORT=/dev/tty.usb[XYZabc]
export RIOTBASE=<path/to/riot/repo>
export PATH=<path/to/gcc-arm-none-eabi>/bin:$PATH
```
Afterwards you should be able to compile the program, flash it onto the board
and connect a terminal via the usb-serial adapter:
```
cd src/[button|pingpong]
make && make flash && make term
```
