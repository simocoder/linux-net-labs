CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDFLAGS :=
BUILD := build
BIN_BLOCKING := $(BUILD)/server_blocking
BIN_FORKED   := $(BUILD)/server_forked

# Make "all" the default goal even if .PHONY appears first
.DEFAULT_GOAL := all

.PHONY: all clean blocking forked

all: $(BIN_BLOCKING) $(BIN_FORKED)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN_BLOCKING): server_blocking.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BIN_FORKED): server_forked.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Convenience aliases
blocking: $(BIN_BLOCKING)
forked:   $(BIN_FORKED)

clean:
	rm -rf $(BUILD)
