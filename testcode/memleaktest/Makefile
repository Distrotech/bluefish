PKGS="gtk+-x11-2.0 gio-2.0 libxml-2.0 gthread-2.0"
CC=gcc
CFLAGS=-Wall -g -O2 `pkg-config --cflags ${PKGS}`
LDFLAGS=`pkg-config --libs ${PKGS}`

SRCS=memleaktest.c

OBJS = $(SRCS:.c=.o)

memleaktest: ${OBJS}
	$(CC) $(DEFS) $(LDFLAGS) $(LIBS) -o $@ ${OBJS}
	
all: memleaktest

clean:
	rm -f memleaktest ${OBJS}

distclean: clean
	rm -f *~

