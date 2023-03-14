CC=gcc
CFLAGS=-g -Wall -O2
TARGET=shttpd

default: $(TARGET)

$(TARGET): main.c const.c conf.o
	$(CC) main.c const.c conf.o -o shttpd $(CFLAGS)

conf.o: conf.c conf.h
	$(CC) conf.c -c -o conf.o $(CFLAGS)

.PHONY: clean

clean:
	-@rm ./*.o
	-@rm $(TARGET)
