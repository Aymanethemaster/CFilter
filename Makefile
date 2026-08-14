CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O3 -Iinclude
TARGET = cfilter.exe
SRCS = src/main.c src/image.c src/filters.c
OBJS = $(SRCS:.c=.o)
INCLUDES = include/image.h include/filters.h include/stb_image.h include/stb_image_write.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) -lm

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS = -Wall -Wextra -std=c99 -g -DDEBUG -Iinclude
debug: $(TARGET)

clean:
	del /Q $(TARGET) src\*.o out.* test_out.* 2>nul || rm -f $(TARGET) src/*.o out.* test_out.*

test: $(TARGET)
	@echo Running automated test suite...
	.\$(TARGET) assets/sample.bmp out.png grayscale
	.\$(TARGET) assets/sample.bmp out.bmp sepia
	.\$(TARGET) assets/sample.bmp out.jpg brightness 40
	.\$(TARGET) assets/sample.bmp out.png rotate 90
	.\$(TARGET) assets/sample.bmp out.png sharpen
	.\$(TARGET) assets/sample.bmp out.png edge
	.\$(TARGET) assets/sample.bmp out.png blur
	@echo All tests executed successfully!

.PHONY: all debug clean test
