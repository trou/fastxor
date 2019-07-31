COPTS=-Wall -O2 -ggdb -D_SSE2=1 -fsanitize=address

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<
