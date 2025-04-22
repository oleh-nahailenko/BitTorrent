TARGET = main

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

# Source and output
SRC = src/main.c src/bencode.c
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Link the object files
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)