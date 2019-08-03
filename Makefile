COPTS=-Wall -O3 -ggdb -Wextra -pedantic -Wconversion -Wsign-conversion

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<

fastxor_check: fastxor.c
	$(CC) $(COPTS) -fsanitize=address -o fastxor $<
