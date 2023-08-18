CXX = clang++
SOURCE = main.cpp
NAME = $(basename $(SOURCE))
BUILD_DIR = build
VERSION = $(shell git describe --tags --always --dirty="-dev" 2>/dev/null || echo "unknown")
MACHINE = $(shell $(CXX) -dumpmachine)
STACK_SIZE = 4294967296
CXXFLAGS = -std=c++20 -O3 -DSTACK_SIZE=$(STACK_SIZE)
ifeq ($(findstring linux,$(MACHINE)), linux)
    EXE_EXT = 
    LDFLAGS = -Wl,-z,stack-size=$(STACK_SIZE)
else ifeq ($(findstring windows-msvc,$(MACHINE)), windows-msvc)
    EXE_EXT = .exe
    LDFLAGS = -Wl,/STACK:$(STACK_SIZE)
else ifeq ($(findstring windows-gnu,$(MACHINE)), windows-gnu)
    EXE_EXT = .exe
    LDFLAGS = -Wl,--stack,$(STACK_SIZE)
else
    $(error Unsupported platform: $(MACHINE))
endif
ifeq ($(USE_GMP), 1)
    CXXFLAGS += -DUSE_GMP
    LDFLAGS += -lgmp
    TARGET = $(BUILD_DIR)/$(NAME)-$(VERSION)-$(MACHINE)-gmpint$(EXE_EXT)
else
    TARGET = $(BUILD_DIR)/$(NAME)-$(VERSION)-$(MACHINE)-strint$(EXE_EXT)
endif
.PHONY: all clean
all: $(TARGET)
$(TARGET): $(SOURCE)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)
clean:
	rm -rf $(BUILD_DIR)
