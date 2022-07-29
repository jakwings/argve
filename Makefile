CFLAGS := -std=c89 -pedantic -Wall -Wextra -Werror -O2 -g -DTEST_H_DEBUGGING
LDFLAGS :=

# e.g. make CC='zig cc --target=x86_64-linux-musl'
ifeq ($(shell uname),Darwin)
CC := xcrun clang
endif

all: test

test: tests/test.sh tests/bin/test1 tests/bin/test2 tests/bin/test3
	./tests/test.sh ./tests/bin/test1
	./tests/bin/test2
	./tests/test.sh ./tests/bin/test3

tests/bin/test1: test.h tests/test1.c Makefile
	mkdir -p tests/bin
	$(CC) $(CFLAGS) $(LDFLAGS) -I. -o tests/bin/test1 tests/test1.c

tests/bin/test2: argve.h test.h tests/test2.c tests/sfc.c Makefile
	mkdir -p tests/bin
	$(CC) $(CFLAGS) $(LDFLAGS) -I. -o tests/bin/test2 tests/test2.c

tests/bin/test3: argve.h test.h tests/test3.c tests/argve.c Makefile
	mkdir -p tests/bin
	$(CC) $(CFLAGS) $(LDFLAGS) -I. -o tests/bin/test3 tests/test3.c tests/argve.c

clean:
	rm -v -R -f -- tests/bin

.PHONEY: clean all test
