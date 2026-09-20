# Makefile - PC 端协议层单元测试
# 跑：make test

CC := gcc
CFLAGS := -Wall -Wextra -O2 -g -std=c11
INCLUDES := -I../../firmware/common

SRC := test_protocol.c ../../firmware/common/protocol.c
BIN := test_protocol

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRC) ../../firmware/common/protocol.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $@

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
