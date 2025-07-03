# Target name
TARGET = build/gart_lang

# Output directory for .o files
BUILD_DIR = build

# Find all C/C++ source files
C_SRCS   := $(shell find . -name "*.c")
CPP_SRCS := $(shell find . -name "*.cpp")
SRCS     := $(C_SRCS) $(CPP_SRCS)

# Convert source paths to object paths inside BUILD_DIR
OBJS := $(patsubst ./%, $(BUILD_DIR)/%, $(SRCS))
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.cpp=.o)

# Compiler setup
CC       = gcc
CXX      = g++
INCLUDES = -Iinclude
CFLAGS   = -Wall -O3 $(INCLUDES)
CXXFLAGS = -std=c++11 -Wall -O3 $(INCLUDES)
LDFLAGS  = 

# Default target
all: $(TARGET)

# Link final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -rf $(BUILD_DIR)