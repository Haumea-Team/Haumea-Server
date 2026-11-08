# haumea-server: Haumea's multiplayer server.
#   make             a static Linux build: build/haumea-server
#   make windows     build/haumea-server.exe (MinGW-w64)
#   make check       the core compiles warning-free, and the binary answers --help
CXX ?= g++
CC ?= gcc
MINGW ?= x86_64-w64-mingw32
CXXFLAGS ?= -O2
WARN = -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
INCLUDES = -Icore
DEFINES = -DHAUMEA_SERVER_VERSION=\"$(VERSION)\"
SOURCES = src/main.cpp src/update.cpp core/mprelay.cpp core/mpaccounts.cpp
C_SOURCES = core/minilzo/minilzo.c core/monocypher/monocypher.c

all: build/haumea-server

build:
	mkdir -p build

build/haumea-server: $(SOURCES) $(C_SOURCES) $(wildcard core/*.h) $(wildcard src/*.h) | build
	$(CC) -O2 -c core/minilzo/minilzo.c -o build/minilzo.o
	$(CC) -O2 -c core/monocypher/monocypher.c -o build/monocypher.o
	$(CXX) -std=c++14 $(CXXFLAGS) $(WARN) $(INCLUDES) $(DEFINES) $(SOURCES) build/minilzo.o build/monocypher.o -static -pthread -o $@
	strip $@

windows: build/haumea-server.exe

build/haumea-server.exe: $(SOURCES) $(C_SOURCES) $(wildcard core/*.h) | build
	$(MINGW)-gcc -O2 -c core/minilzo/minilzo.c -o build/minilzo-w.o
	$(MINGW)-gcc -O2 -c core/monocypher/monocypher.c -o build/monocypher-w.o
	$(MINGW)-g++ -std=c++14 $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(SOURCES) build/minilzo-w.o build/monocypher-w.o \
		-lws2_32 -static -static-libgcc -static-libstdc++ -o $@

check: build/haumea-server
	./build/haumea-server --help > /dev/null
	@echo "haumea-server $$(grep -o 'MP_PROTOCOL = [0-9]*' core/mpprotocol.h): ok"

clean:
	rm -rf build

.PHONY: all windows check clean
