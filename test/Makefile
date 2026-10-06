# Testsuite target.
TARGET := testsuite

# Source, include and framework directories.
SOURCE_DIR := ../source
INC_DIR    := ../include
TEST_DIR   := ../libs/test
TEST_LIB   := $(TEST_DIR)/libyrgo_test.a

# Driver sources (C) and test sources (C++). Discovered automatically, so adding a driver or a
# test does not mean editing this file.
C_SOURCES   := $(shell find $(SOURCE_DIR) -name '*.c')
CXX_SOURCES := $(shell find . -name '*.cpp')

# Object files.
C_OBJ   := $(C_SOURCES:.c=.o)
CXX_OBJ := $(CXX_SOURCES:.cpp=.o)

# Compilers.
CC  := gcc
CXX := g++

# Flags shared by both languages. TESTSUITE selects the mocked hardware platform.
COMMON_FLAGS := -Wall -Wextra -Werror -I$(INC_DIR) -DTESTSUITE

# Language specific compiler flags.
C_FLAGS   := -std=c11 $(COMMON_FLAGS)
CXX_FLAGS := -std=c++17 $(COMMON_FLAGS) -I$(TEST_DIR)/include

.PHONY: default build run clean

# Build and run the test suite as default.
default: build run

# Build the yrgo::test library.
$(TEST_LIB):
	@$(MAKE) -C $(TEST_DIR) lib

# Compile the C drivers.
%.o: %.c
	@$(CC) $(C_FLAGS) -c $< -o $@

# Compile the C++ tests.
%.o: %.cpp
	@$(CXX) $(CXX_FLAGS) -c $< -o $@

# Link the test suite. The C objects are linked into the C++ binary, so g++ drives the link.
build: $(TEST_LIB) $(C_OBJ) $(CXX_OBJ)
	@$(CXX) $(C_OBJ) $(CXX_OBJ) -o $(TARGET) -L$(TEST_DIR) -lyrgo_test

# Run the test suite.
run:
	@./$(TARGET)

# Clean the test suite, the yrgo::test library and any coverage artifacts.
clean:
	@rm -f $(TARGET) $(C_OBJ) $(CXX_OBJ) *.gcda *.gcno
	@$(MAKE) -C $(TEST_DIR) clean
