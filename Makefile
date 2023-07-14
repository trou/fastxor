COPTS=-Wall -O3 -DUSE_SSE=1 -ggdb -Wextra -pedantic -Wconversion -Wsign-conversion -march=native --std=c11 -Werror -Wall -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3  -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fPIE -fsanitize=bounds -fsanitize-undefined-trap-on-error -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code

fastxor: fastxor.c
	$(CC) $(COPTS) -o fastxor $<

fastxor_clang: fastxor.c
	clang $(COPTS) -o fastxor $<

fastxor_check: fastxor.c
	$(CC) $(COPTS) -fsanitize=undefined -fsanitize=address -o fastxor $<

clean:
	rm -f fastxor
