CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDFLAGS :=
BUILD := build
BIN := $(BUILD)/server_blocking

.PHONY: all clean
all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN): server_blocking.c | $(BUILD)
	$(CC) $(CFLAGS) -o $(BIN) server_blocking.c $(LDFLAGS)

clean:
	rm -rf $(BUILD)
