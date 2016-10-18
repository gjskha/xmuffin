LIBS = -L/usr/X11R6/lib -lXext -lX11 -lcurl
CFLAGS = -O2 -fomit-frame-pointer -pipe -Wall
#CFLAGS = -g -pipe

all: xmuffin

xmuffin: xmuffin.o socket.o
	gcc $(CFLAGS) -o xmuffin xmuffin.o socket.o $(LIBS)

xmuffin.o: xmuffin.c defaults.h
	gcc $(CFLAGS) -c xmuffin.c

socket.o: socket.c defaults.h
	gcc $(CFLAGS) -c socket.c

dist: clean
	@(dir=`pwd`; name=`basename $$dir`; \
	rm -f $$name.tar.gz ; \
	cd .. ; tar -zcf $$name/$$name.tar.gz --exclude $$name/$$name.tar.gz $$name )

clean:
	-rm -f *.o xmuffin
