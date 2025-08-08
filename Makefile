# Makefile for rom2msx
# Bygger en CLI-konverter til MSX-ROM layout for MegaSCC, RC755 og Simple64K
# Default compiler-indstillinger er portable til Unix/Mac/Linux.

CXX      := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra
TARGET   := rom2msx
SRC      := rom2msx.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

