#
# Example program
#
# Makefile used to build the software
#
# Copyright 2014-2015 Nicolas Mora <mail@babelouest.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the MIT License
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
LIBSUNXI_LOCATION=../lib/librf24-sunxi/lib
LIBSUNXI_LOCATION_SRC=../lib/librf24-sunxi/src
LIBYDER_LOCATION=../lib/ulfius/lib/yder/src
CC=g++
ADDITIONALFLAGS=-Wno-write-strings
CFLAGS=-c -Wall -I$(LIBYDER_LOCATION) -I$(LIBSUNXI_LOCATION_SRC) -D_REENTRANT $(ADDITIONALFLAGS) 
ULFIUS_LOCATION=../lib/ulfius/src
LIBS=-L$(LIBYDER_LOCATION) -lc -lulfius -lyder -ljansson -L$(ULFIUS_LOCATION) -L$(LIBSUNXI_LOCATION) -lrf24-sunxi -lrrd


all: rest_server

clean:
	rm -f *.o rest_server

debug: ADDITIONALFLAGS=-DDEBUG -g -O0

debug: rest_server

libulfius.so:
	cd $(ULFIUS_LOCATION) && $(MAKE) debug

rest_server.o: rest_server.cpp
	$(CC) $(CFLAGS) rest_server.cpp -DDEBUG -g -O0

rest_server: libulfius.so rest_server.o
	$(CC) -o rest_server rest_server.o $(LIBS)

rest: rest_server
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):${LD_LIBRARY_PATH} ./rest_server