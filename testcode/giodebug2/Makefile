PKGS="glib-2.0 gio-2.0"
CC=gcc
CFLAGS=-Wall -g -O2 `pkg-config --cflags ${PKGS}`
LDFLAGS=`pkg-config --libs ${PKGS}`

SRCS=giodebug.c

OBJS = $(SRCS:.c=.o)

giodebug: ${OBJS}
	$(CC) $(DEFS) $(LDFLAGS) $(LIBS) -o $@ ${OBJS}
	
all: giodebug

clean:
	rm -f giodebug ${OBJS}

distclean: clean
	rm -f *~

