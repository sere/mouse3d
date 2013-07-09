#
# Makefile for serial examples
# This is an automated build system for GNU/Linux
#
# Compiler: gcc
# Linker: ld
#

all: read_serial

read_serial: read_serial.c
	gcc -Wall -o read_serial read_serial.c

clean:
	rm -f read_serial
