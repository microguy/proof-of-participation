# GoldcoinPoP - The Greatest Money in the Galaxy
# Copyright (c) 2025 GoldcoinPoP Developers
# Following Satoshi's vision with modern tools

CXX = g++-13
CXXFLAGS = -std=c++23 -O2 -Wall -Wextra -Wpedantic \
           -fno-exceptions -fno-rtti \
           -march=native -mtune=native \
           -DNDEBUG -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

# Debug build
ifdef DEBUG
CXXFLAGS = -std=c++23 -g -O0 -Wall -Wextra -Wpedantic \
           -fsanitize=address,undefined \
           -D_DEBUG -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
endif

# Berkeley DB 18.1.40
BDB_PREFIX = /usr/local/BerkeleyDB.18.1
BDB_INCLUDE = -I$(BDB_PREFIX)/include
BDB_LIB = -L$(BDB_PREFIX)/lib -ldb_cxx-18.1

# Boost libraries (modern versions)
BOOST_INCLUDE = -I/usr/local/include
BOOST_LIB = -L/usr/local/lib \
            -lboost_system \
            -lboost_filesystem \
            -lboost_thread \
            -lboost_chrono

# OpenSSL
CRYPTO_LIB = -lcrypto -lssl

# Threading
THREAD_LIB = -lpthread

# All includes
INCLUDES = $(BDB_INCLUDE) $(BOOST_INCLUDE) -I.

# All libraries  
LIBS = $(BDB_LIB) $(BOOST_LIB) $(CRYPTO_LIB) $(THREAD_LIB)

# Headers (modern C++23 modules when available)
HEADERS = \
    uint256.h \
    bignum.h \
    base58.h \
    serialize.h \
    util.h \
    key.h \
    script.h \
    db.h \
    net.h \
    irc.h \
    main.h \
    rpc.h \
    init.h \
    sha.h \
    participation.h \
    vrf.h

# Source files
SOURCES = \
    util.cpp \
    script.cpp \
    db.cpp \
    net.cpp \
    irc.cpp \
    main.cpp \
    rpc.cpp \
    init.cpp \
    sha.cpp \
    participation.cpp \
    vrf.cpp

# Object files
OBJS = $(SOURCES:%.cpp=obj/%.o)

# Executable name
TARGET = goldcoinpop

# Build directories
OBJDIR = obj
BINDIR = bin

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

# Main executable
$(TARGET): $(OBJS)
	@echo "Linking GoldcoinPoP - Building the future of money..."
	$(CXX) $(CXXFLAGS) -o $(BINDIR)/$@ $^ $(LIBS)
	@echo "✓ GoldcoinPoP built successfully!"
	@echo "Satoshi would be proud - energy-efficient consensus achieved."

# Compile source files with C++23
$(OBJDIR)/%.o: %.cpp $(HEADERS)
	@echo "Compiling $< with C++23..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Special optimization for SHA256 (Satoshi's optimization preserved)
$(OBJDIR)/sha.o: sha.cpp sha.h
	@echo "Compiling SHA256 with maximum optimization..."
	$(CXX) $(CXXFLAGS) -O3 -march=native $(INCLUDES) -c $< -o $@

# Special handling for participation system
$(OBJDIR)/participation.o: participation.cpp participation.h
	@echo "Compiling Proof of Participation..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJDIR)/*.o
	@rm -rf $(BINDIR)/$(TARGET)
	@echo "✓ Clean complete"

# Full rebuild
rebuild: clean all

# Install (requires sudo)
install: all
	@echo "Installing GoldcoinPoP..."
	@sudo cp $(BINDIR)/$(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✓ GoldcoinPoP installed to /usr/local/bin/"

# Development helpers
.PHONY: all clean rebuild install directories

# Static analysis with C++23 features
analyze:
	@echo "Running static analysis..."
	@clang-tidy-15 $(SOURCES) -- $(CXXFLAGS) $(INCLUDES)

# Format code to modern C++ standards
format:
	@echo "Formatting code..."
	@clang-format-15 -i $(SOURCES) $(HEADERS) --style=file

# Generate compile_commands.json for IDEs
compile_commands:
	@bear -- make

# Test build with all warnings
pedantic:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -Werror -pedantic-errors"

# Profile-guided optimization build
pgo-generate:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -fprofile-generate"

pgo-use:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -fprofile-use"

# What would Satoshi do?
wisdom:
	@echo "What would Satoshi do?"
	@echo "1. Keep it simple"
	@echo "2. No trusted third parties"  
	@echo "3. Robust unstructured simplicity"
	@echo "4. One CPU one vote → One stake one vote"
	@echo "5. Messages on best effort basis"
	@echo "6. Nodes can leave and rejoin at will"
	@echo "7. Privacy through anonymity, not obscurity"

help:
	@echo "GoldcoinPoP Build System"
	@echo "========================"
	@echo "make          - Build GoldcoinPoP"
	@echo "make DEBUG=1  - Build with debug symbols"
	@echo "make clean    - Clean build artifacts"
	@echo "make rebuild  - Clean and rebuild"
	@echo "make install  - Install to system (requires sudo)"
	@echo "make analyze  - Run static analysis"
	@echo "make format   - Format code"
	@echo "make wisdom   - Remember Satoshi's principles"