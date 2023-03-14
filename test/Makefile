CC=gcc
CFLAGS=-g -Wall
SRC=..

default: run

.PHONY: clean

clean:
	-@rm ./*.o
	-@rm ./unit_test

run:unit_test
	./unit_test

unit_test:unit_test.c $(SRC)/const.c conf.o cutest.o
	$(CC) unit_test.c $(SRC)/const.c conf.o cutest.o -o unit_test $(CFLAGS)

cutest.o: CuTest.c CuTest.o
	$(CC) CuTest.c -c -o cutest.o $(CFLAGS)

conf.o: $(SRC)/conf.c $(SRC)/conf.h
	$(CC) $(SRC)/conf.c -c -o conf.o $(CFLAGS)


