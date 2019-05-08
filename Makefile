TARGET = micropak

CXX = g++
CPPFLAGS = -Wall -g -std=c++17 

all: archiver.h archiver.cpp
	$(CXX) $(CPPFLAGS) archiver.cpp -o $(TARGET) -lstdc++fs
clean:
	rm $(TARGET)
