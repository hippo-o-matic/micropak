TARGET = micropak

CXX = g++
CPPFLAGS = -Wall -std=c++17 -I. -L.
LIBS = -lstdc++fs -lz

# Remove -lz if you don't want gzip and don't have zlib installed
all: main.cpp libmicropak.a
	$(CXX) $(CPPFLAGS) main.cpp -o $(TARGET) -lmicropak $(LIBS)

libmicropak.a: micropak.o
	ar rcs libmicropak.a micropak.o

micropak.o: micropak.hpp micropak.cpp
	$(CXX) $(CPPFLAGS) -O -c micropak.hpp micropak.cpp $(LIBS)

clean:
	rm -f *.o *.a $(TARGET) *.gch
