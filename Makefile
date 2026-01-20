# Makefile for 3D Rotating Cube Renderer
# Supports multiple platforms: Linux, macOS, Windows (MinGW, MSVC)

# Project configuration
TARGET = cube
SOURCE = cube.c
INSTALL_DIR = /usr/local/bin

# Detect operating system
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(UNAME_S)
endif

# Compiler and flags based on OS
ifeq ($(DETECTED_OS),Windows)
    # Windows with MinGW
    CC = gcc
    CFLAGS = -Wall -Wextra -O2 -std=c99
    LDFLAGS = -lm
    TARGET_EXT = .exe
    RM = del /Q
    MKDIR = mkdir
else ifeq ($(UNAME_S),Darwin)
    # macOS
    CC = clang
    CFLAGS = -Wall -Wextra -O2 -std=c99
    LDFLAGS = -lm
    TARGET_EXT =
    RM = rm -f
    MKDIR = mkdir -p
else
    # Linux and other Unix-like systems
    CC = gcc
    CFLAGS = -Wall -Wextra -O2 -std=c99
    LDFLAGS = -lm
    TARGET_EXT =
    RM = rm -f
    MKDIR = mkdir -p
endif

# Final target name
TARGET_FINAL = $(TARGET)$(TARGET_EXT)

# Default target
all: $(TARGET_FINAL)

# Build target
$(TARGET_FINAL): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET_FINAL)

# Release build with optimization
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET_FINAL)

# Install target (Unix-like systems only)
ifeq ($(DETECTED_OS),Windows)
install:
	@echo "Install target not supported on Windows. Copy $(TARGET_FINAL) manually."
else
install: $(TARGET_FINAL)
	$(MKDIR) $(INSTALL_DIR)
	cp $(TARGET_FINAL) $(INSTALL_DIR)/
	@echo "Installed $(TARGET_FINAL) to $(INSTALL_DIR)"
endif

# Uninstall target (Unix-like systems only)
ifeq ($(DETECTED_OS),Windows)
uninstall:
	@echo "Uninstall target not supported on Windows. Remove $(TARGET_FINAL) manually."
else
uninstall:
	$(RM) $(INSTALL_DIR)/$(TARGET_FINAL)
	@echo "Removed $(TARGET_FINAL) from $(INSTALL_DIR)"
endif

# Clean build artifacts
clean:
ifeq ($(DETECTED_OS),Windows)
	-$(RM) $(TARGET_FINAL) *.o *.obj
else
	-$(RM) $(TARGET_FINAL) *.o
endif

# Run the program
run: $(TARGET_FINAL)
	./$(TARGET_FINAL)

# Check for required dependencies
check:
	@echo "Checking build environment..."
	@which $(CC) > /dev/null || (echo "Error: $(CC) not found. Please install $(CC)." && exit 1)
	@echo "Compiler: $(CC)"
	@echo "Operating System: $(DETECTED_OS)"
	@echo "Build configuration ready."

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build the cube program (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run the program"
	@echo "  check     - Check build environment"
	@echo "  install   - Install to system (Unix-like only)"
	@echo "  uninstall - Remove from system (Unix-like only)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Detected OS: $(DETECTED_OS)"
	@echo "Compiler: $(CC)"
	@echo "Target: $(TARGET_FINAL)"

# Declare phony targets
.PHONY: all debug release clean install uninstall run check help

# Default goal
.DEFAULT_GOAL := all