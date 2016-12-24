# Orange Pi - NostraDomus

## INSTALLATION ##

Follow the instructions below.

## Prerequisites

### External dependencies

To install the external dependencies, for Debian based distributions (Debian, Ubuntu, Raspbian, etc.), run as root:

```shell
# apt-get install libmicrohttpd-dev libjansson-dev libcurl4-gnutls-dev
```

Download the two libraries:

1) librf24-sunxi
```shell
	git clone https://github.com/bearpawmaxim/librf24-sunxi.git
```

2) ulfius
```shell
	git clone https://github.com/babelouest/ulfius.git
```	


### Fix librf24-sunxi ###

Edit Makefile and add ${PREFIX} at line 39 so that:

	@ln -sf ${LIBDIR}${LIBNAME} ${PREFIX}${LIBDIR}${LIB}.so.1

	into

	@ln -sf ${PREFIX}${LIBDIR}${LIBNAME} ${PREFIX}${LIBDIR}${LIB}.so.1

### Make & install the libraries ###



