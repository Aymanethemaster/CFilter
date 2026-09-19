CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O3 -Iinclude

# Executable/file suffix (".exe" on Windows, empty elsewhere)
ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

TARGET = cfilter$(EXE)
SRCS = src/main.c src/image.c src/filters.c
OBJS = $(SRCS:.c=.o)
INCLUDES = include/image.h include/filters.h include/stb_image.h include/stb_image_write.h

FIXTURE_GEN = tests/generate_fixture$(EXE)
TEST_RUNNER = tests/test_filters$(EXE)
TEST_SRCS = tests/test_filters.c src/image.c src/filters.c

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) -lm

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS = -Wall -Wextra -std=c99 -g -DDEBUG -Iinclude
debug: $(TARGET)

# Deterministic test fixture, generated on the fly (BMP round-trip self-verified)
tests/fixture.bmp: tests/generate_fixture.c tests/fixture_pixels.h
	$(CC) $(CFLAGS) tests/generate_fixture.c -o $(FIXTURE_GEN) -lm
	$(FIXTURE_GEN) tests/fixture.bmp

$(TEST_RUNNER): $(TEST_SRCS) $(INCLUDES)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_RUNNER) -lm

test: $(TARGET) $(TEST_RUNNER) tests/fixture.bmp
	$(TEST_RUNNER) tests/fixture.bmp ./$(TARGET)

clean:
ifeq ($(OS),Windows_NT)
	-del /Q $(TARGET) src\*.o $(FIXTURE_GEN) $(TEST_RUNNER) tests\fixture.bmp tmp_out.* 2>nul
else
	rm -f $(TARGET) src/*.o $(FIXTURE_GEN) $(TEST_RUNNER) tests/fixture.bmp tmp_out.*
endif

.PHONY: all debug test clean
