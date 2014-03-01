CC = gcc
C_FLAGS = -DHAVE_XPM -Wall -I/usr/X11R6/include
L_FLAGS = -L/usr/X11R6/lib -lX11 -lXpm
EXTRA_CFLAGS = -g -Os
PROGNAME = fspanel

$(PROGNAME): Makefile fspanel.c fspanel.h icon.xpm
	$(CC) $(C_FLAGS) $(EXTRA_CFLAGS) fspanel.c -o $(PROGNAME) $(L_FLAGS)

clean:
	rm -f core *.o $(PROGNAME) nohup.out

install: $(PROGNAME)
	install -d -m 755 $(DESTDIR)/usr/bin
	install -m 755 $(PROGNAME) $(DESTDIR)/usr/bin
	install -d -m 755 $(DESTDIR)/usr/share/man/man1

