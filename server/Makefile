CC = gcc
CFLAGS = -Wall -Wextra -I./include `pkg-config --cflags libmongoc-1.0`
LDFLAGS = `pkg-config --libs libmongoc-1.0` -lssl -lcrypto -lws2_32

SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c) \
          $(wildcard $(SRC_DIR)/auth/*.c) \
          $(wildcard $(SRC_DIR)/database/*.c) \
          $(wildcard $(SRC_DIR)/utils/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executable
TARGET = $(BUILD_DIR)/battleship_server.exe

# Default target
all: $(TARGET)

# Create directories
$(OBJ_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
	@if not exist "$(OBJ_DIR)\auth" mkdir "$(OBJ_DIR)\auth"
	@if not exist "$(OBJ_DIR)\database" mkdir "$(OBJ_DIR)\database"
	@if not exist "$(OBJ_DIR)\utils" mkdir "$(OBJ_DIR)\utils"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Clean
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"

# Run
run: $(TARGET)
	$(TARGET)

.PHONY: all clean run