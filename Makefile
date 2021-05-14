DESTDIR ?=
PREFIX ?= /usr/local

MAKEFLAGS := -k

CFLAGS := -O2 -Wall -Wno-parentheses

CPPFLAGS += \
	-Iinclude \
	-DAF_CAN=PF_CAN \
	-DPF_CAN=29 \
	-DSO_RXQ_OVFL=40 \
	-DSCM_TIMESTAMPING_OPT_STATS=54 \
	-D_FILE_OFFSET_BITS=64 \
	-D_GNU_SOURCE


PROGRAMS_XCP := xcpdump


PROGRAMS := xcpdump

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.o

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(PROGRAMS) $(DESTDIR)$(PREFIX)/bin

distclean:
	rm -f $(PROGRAMS) $(LIBRARIES) *.o *~

xcpdump:	xcpdump.o	xcpdissect.o
