COPTS=-Wall -O2 -ggdb

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<
