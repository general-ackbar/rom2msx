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

