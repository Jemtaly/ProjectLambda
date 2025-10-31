CXX = clang++
SOURCE = main.cpp
NAME = $(basename $(SOURCE))
BUILD_DIR = build
VERSION = $(shell git describe --tags --always --dirty="-dev" 2>/dev/null || echo "unknown")
MACHINE = $(shell $(CXX) -dumpmachine)
CXXFLAGS = -std=c++20 -O3

ifeq ($(findstring linux,$(MACHINE)), linux)
    EXE_EXT =
    LDFLAGS =
else ifeq ($(findstring windows-msvc,$(MACHINE)), windows-msvc)
    EXE_EXT = .exe
    LDFLAGS =
else ifeq ($(findstring windows-gnu,$(MACHINE)), windows-gnu)
    EXE_EXT = .exe
    LDFLAGS =
else
    $(error Unsupported platform: $(MACHINE))
endif

ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -fsanitize=address -fsanitize=undefined
    VERSION := $(VERSION)-debug
endif

ifeq ($(USE_GMP), 1)
    CXXFLAGS += -DUSE_GMP
    LDFLAGS += -lgmp
    TARGET = $(BUILD_DIR)/$(NAME)-$(VERSION)-$(MACHINE)-gmp$(EXE_EXT)
else
    TARGET = $(BUILD_DIR)/$(NAME)-$(VERSION)-$(MACHINE)-nat$(EXE_EXT)
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
