CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude
PREFIX ?= /usr/local

TARGET = bin/sandbox
SRCS = src/main.c src/cli.c src/isolate.c src/landlock_mgr.c src/seccomp_bpf.c
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
HEADERS = include/sandbox.h include/cli.h include/isolate.h include/landlock_mgr.h include/seccomp_bpf.h

BENCH_TARGET = benchmarks/bench_timer
BENCH_SRC = benchmarks/bench_timer.c

.PHONY: all clean test bench install uninstall

all: $(TARGET)

$(TARGET): $(OBJS) | bin
	$(CC) $(CFLAGS) $(OBJS) -o $@

build/%.o: src/%.c $(HEADERS) | build
	$(CC) $(CFLAGS) -c $< -o $@

bin build:
	mkdir -p $@

clean:
	rm -rf bin build $(BENCH_TARGET) /tmp/sb_*

test: $(TARGET)
	@chmod +x tests/*.sh
	@./tests/run_all.sh

$(BENCH_TARGET): $(BENCH_SRC)
	$(CC) -O2 $(BENCH_SRC) -o $@ -lm

bench: $(TARGET) $(BENCH_TARGET)
	@chmod +x benchmarks/*.sh
	@./benchmarks/bench_startup.sh

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/sandbox

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sandbox
