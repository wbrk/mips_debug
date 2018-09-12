CC := gcc
CFLAGS := -Wall -Wextra
TARGET := test

SRC := $(wildcard *.c)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm $(TARGET)

.PHONY: all clean
