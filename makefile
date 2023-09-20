# Makefile for Hello World program

# Compiler
CC = g++

# Compiler flags
CCFLAGS = -Wall -Wextra -std=c++11

# Target executable (outside the folders)
TARGET = compiler

# Directories
SRC_DIR = src
HEADER_DIR = header
OBJ_DIR = obj

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object files (automatically generated in the obj folder)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CCFLAGS) -o $@ $^

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
# create obj dir if doesn't exist
	@mkdir -p $(@D)
	$(CC) $(CCFLAGS) -I$(HEADER_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
