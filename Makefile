TARGET = micropak

CXX = g++
CPPFLAGS = -Wall -g -std=c++17 

all: micropak.h micropak.cpp
	$(CXX) $(CPPFLAGS) micropak.cpp -o $(TARGET) -lstdc++fs
clean:
	rm $(TARGET)
