CC = gcc
CFLAGS = -Wall -I/usr/X11R6/include -std=c99
LFLAGS = -L/usr/X11R6/lib -lX11
TARGET = craftbar

SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

.PRECIOUS: $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o $(TARGET) .depend

install: $(TARGET)
	install -d -m 755 $(DESTDIR)/usr/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/bin

depend: .depend

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ -MF  ./.depend

include .depend
