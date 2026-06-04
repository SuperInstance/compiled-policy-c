# compiled-policy-c — Zero-dependency compiled tile policies for microcontrollers
#
# Targets: test, bench, examples, clean
#
# CC and CFLAGS can be overridden for cross-compilation:
#   make CC=arm-none-eabi-gcc CFLAGS="-O2 -mcpu=cortex-m4"

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2 -std=c99
LDFLAGS ?= -lm

# Source files
LIB_SRC = compiled_policy.c
LIB_HDR = compiled_policy.h ttt_policy_data.h c4_policy_data.h

.PHONY: all test bench examples clean

all: examples

# --- Library object ---
compiled_policy.o: compiled_policy.c compiled_policy.h
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Tests ---
test_lookup: test_lookup.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ test_lookup.c compiled_policy.o $(LDFLAGS)

test_hash: test_hash.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ test_hash.c compiled_policy.o $(LDFLAGS)

test_cross_language: test_cross_language.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ test_cross_language.c compiled_policy.o $(LDFLAGS)

test: test_lookup test_hash test_cross_language
	@echo ""
	@echo "=== Running tests ==="
	@./test_hash
	@echo ""
	@./test_lookup
	@echo ""
	@./test_cross_language

# --- Benchmarks ---
benchmark: benchmark.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ benchmark.c compiled_policy.o $(LDFLAGS)

bench: benchmark
	@echo ""
	@./benchmark

# --- Examples ---
ttt_main: ttt_main.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ ttt_main.c compiled_policy.o $(LDFLAGS)

c4_main: c4_main.c compiled_policy.o $(LIB_HDR)
	$(CC) $(CFLAGS) -o $@ c4_main.c compiled_policy.o $(LDFLAGS)

examples: ttt_main c4_main
	@echo ""
	@echo "=== TTT Example ==="
	@./ttt_main
	@echo ""
	@echo "=== C4 Example ==="
	@./c4_main

# --- Clean ---
clean:
	rm -f *.o test_lookup test_hash test_cross_language benchmark ttt_main c4_main

# --- Cross-compilation helpers ---
esp8266:
	$(MAKE) CC=xtensa-lx106-elf-gcc CFLAGS="-Os -std=c99 -Wall -I." LDFLAGS=""

arm:
	$(MAKE) CC=arm-none-eabi-gcc CFLAGS="-Os -std=c99 -Wall -mcpu=cortex-m4" LDFLAGS=""
