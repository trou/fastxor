COPTS=-Wall -O2 -ggdb

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<

fastxor_sse: fastxor.c
	$(CC) $(COPTS) -D_SSE2=1 -o fastxor $<

fastxor_check: fastxor.c
	$(CC) $(COPTS) -fsanitize=address -o fastxor $<
