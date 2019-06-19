TARGET = micropak

CXX = g++
CPPFLAGS = -Wall -g -std=c++17 -I.

# Remove -lz if you don't want gzip and don't have zlib installed
all: micropak.h micropak.cpp
	$(CXX) $(CPPFLAGS) micropak.cpp -o $(TARGET) -lstdc++fs -lz
clean:
	rm $(TARGET)
