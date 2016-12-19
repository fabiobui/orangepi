LIBNAME=rf24-sunxi
LIBDIR=../lib/librf24-sunxi/lib/
SRCDIR=../lib/librf24-sunxi/src/


LIBSUNXI_LOCATION=../../../librf24-sunxi/lib
LIBSUNXI_LOCATION_SRC=../../../librf24-sunxi/src

LIBYDER_LOCATION=../../lib/yder/src
CC=g++
CFLAGS=-c -Wall -I$(LIBYDER_LOCATION) -I$(LIBSUNXI_LOCATION_SRC) -D_REENTRANT $(ADDITIONALFLAGS)
ULFIUS_LOCATION=../../src
LIBS=-L$(LIBSUNXI_LOCATION) -lrf24-sunxi


all:spidev_test pingtest scanner arduino_rcv arduino_rcv_irq

test:
	g++ attiny_receive.cpp -I${SRCDIR} $(LIBS) -o attiny_receive	

receive:
	g++ attiny_receive.cpp -I${SRCDIR} -L${LIBDIR} -l${LIBNAME} -o attiny_receive

send:
	g++ attiny_send.cpp -I${SRCDIR} -L${LIBDIR} -l${LIBNAME} -o attiny_send

clean:
	rm -f attiny_receive attiny_send

