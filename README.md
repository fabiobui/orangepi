# Orange Pi - NostraDomus

## INSTALLATION ##

Follow the instructions below.

## Prerequisites

### External dependencies

To install the external dependencies, for Debian based distributions (Debian, Ubuntu, Raspbian, etc.), run as root:

```shell
$ apt-get install libmicrohttpd-dev libjansson-dev libcurl4-gnutls-dev
```

Download the two libraries:

```shell
$ cd orangepi/lib
```

1) librf24-sunxi
```shell
$ git clone https://github.com/bearpawmaxim/librf24-sunxi.git
```

2) ulfius
```shell
$ git clone https://github.com/babelouest/ulfius.git
```	


### Fix librf24-sunxi ###

Edit Makefile and add ${PREFIX} at line 39 so that:

	@ln -sf ${LIBDIR}${LIBNAME} ${PREFIX}${LIBDIR}${LIB}.so.1

	into

	@ln -sf ${PREFIX}${LIBDIR}${LIBNAME} ${PREFIX}${LIBDIR}${LIB}.so.1

### Make & install the libraries ###

Compiling librf24-sunxi

```shell
$ cd librf24-sunxi
$ make
$ make install
```

Download Ulfius source code from Github, get the submodules, compile and install:

```shell
$ git clone https://github.com/babelouest/ulfius.git
$ cd ulfius/
$ git submodule update --init
$ make
$ sudo make install
```
### Install rrdtool versione 1.6 or greater ###

Since rrdtool v.1.4.8 debian jessie package dosen't work whit rest_server application, you need install last version avaible coming from testing repos.
Add testing repos to /etc/apt/sources.list:

```shell
deb     http://ftp.de.debian.org/debian/    testing main contrib non-free
deb-src http://ftp.de.debian.org/debian/    testing main contrib non-free

deb     http://security.debian.org/         testing/updates  main contrib non-free
```

Create the following file /etc/apt/99defaultrelease with these content:

```shell
APT::Default-Release "stable";
```

After that changes:

```shell
$ apt-get update
$ apt-get install rrdtool=1.6.0
```
