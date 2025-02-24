# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include  # Include the headers from 'include/'

# Directories
SRC_DIR = src
OBJ_DIR = build
INCLUDE_DIR = include

# Files
SRCS = $(wildcard $(SRC_DIR)/*.c)      # All .c files in src/
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)  # Corresponding .o files in build/

# Target executable
TARGET = starpu_app

# Default rule
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

# Rule to build object files (.o) from .c files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule to remove build artifacts
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)

# Phony targets
.PHONY: all clean
