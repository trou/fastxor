GCC_COPTS=-Wall -O3 -DUSE_SSE=1 -ggdb -Wextra -pedantic -Wconversion -Wsign-conversion -march=native --std=c11 -Werror -Wall -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3  -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -fPIE -fsanitize=bounds -fsanitize-undefined-trap-on-error -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code

CLANG_COPTS=-O3 -Werror -Walloca -Wcast-qual -Wconversion -Wformat=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wvla -Warray-bounds -Warray-bounds-pointer-arithmetic -Wassign-enum -Wbad-function-cast -Wconditional-uninitialized -Wconversion -Wfloat-equal -Wformat-type-confusion -Widiomatic-parentheses -Wimplicit-fallthrough -Wloop-analysis -Wpointer-arith -Wshift-sign-overflow -Wshorten-64-to-32 -Wswitch-enum -Wtautological-constant-in-range-compare -Wunreachable-code-aggressive -Wthread-safety -Wthread-safety-beta -Wcomma -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fsanitize=safe-stack -fPIE -fstack-clash-protection -fsanitize=bounds -fsanitize-undefined-trap-on-error -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code

fastxor: fastxor.c
	gcc $(GCC_COPTS) -o fastxor $<

fastxor_clang: fastxor.c
	clang $(CLANG_COPTS) -o fastxor $<

fastxor_check: fastxor.c
	gcc $(GCC_COPTS) -fsanitize=undefined -fsanitize=address -o fastxor $<

clean:
	rm -f fastxor
