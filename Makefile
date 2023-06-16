CC=gcc
RELEASE=-DNDEBUG
DEBUG=-g
CFLAGS=-Wall $(DEBUG)
TARGET=shttpd

default: $(TARGET)

$(TARGET): main.c const.c conf.o http.o common.h
	$(CC) main.c const.c conf.o http.o -o shttpd $(CFLAGS)

conf.o: conf.c conf.h common.h
	$(CC) conf.c -c -o conf.o $(CFLAGS)

http.o: http.c http.h common.h
	$(CC) http.c -c -o http.o $(CFLAGS)

.PHONY: clean

clean:
	-@rm ./*.o
	-@rm $(TARGET)
