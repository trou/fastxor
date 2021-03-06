COPTS=-Wall -O3 -ggdb -Wextra -pedantic -Wconversion -Wsign-conversion -march=native --std=c11

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<

fastxor_check: fastxor.c
	$(CC) $(COPTS) -fsanitize=undefined -fsanitize=address -o fastxor $<
