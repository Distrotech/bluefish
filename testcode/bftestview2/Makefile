PKGS="gtk+-x11-2.0 gio-2.0"
CC=gcc
CFLAGS=-g -O2 `pkg-config --cflags ${PKGS}`
LDFLAGS=`pkg-config --libs ${PKGS}`

SRCS=bftextview2.c bftextview2_scanner.c bftextview2_patcompile.c bftextview2_autocomp.c testapp.c

OBJS = $(SRCS:.c=.o)

testapp: ${OBJS}
	$(CC) $(DEFS) $(LDFLAGS) $(LIBS) -o $@ ${OBJS}
	
all: testapp

clean:
	rm -f testapp ${OBJS}

distclean: clean
	rm -f *~

