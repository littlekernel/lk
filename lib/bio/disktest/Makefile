CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11

TARGET := disktest
SRCS := disktest.c disktest_posix_backend.c

.PHONY: all clean test-corruption

all: $(TARGET)

$(TARGET): $(SRCS) disktest_backend.h
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)

test-corruption: $(TARGET)
	./test_corruption.sh
