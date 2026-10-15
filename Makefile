CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude
PREFIX ?= /usr/local

TARGET = bin/sandbox
SRCS = src/main.c src/cli.c src/isolate.c src/landlock_mgr.c src/seccomp_bpf.c
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

.PHONY: all clean test bench install uninstall

all: $(TARGET)

$(TARGET): $(OBJS) | bin
	$(CC) $(CFLAGS) $(OBJS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

bin build:
	mkdir -p $@

clean:
	rm -rf bin build /tmp/sb_test_*

test: $(TARGET)
	@chmod +x tests/*.sh
	@./tests/run_all.sh

bench: $(TARGET)
	@chmod +x benchmarks/*.sh
	@./benchmarks/bench_startup.sh

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/sandbox

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sandbox
