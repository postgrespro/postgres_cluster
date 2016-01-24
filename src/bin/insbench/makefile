CXX=g++
CXXFLAGS=-g -Wall -O2 -pthread 

all: insbench

insbench: insbench.cpp
	$(CXX) $(CXXFLAGS) -o insbench insbench.cpp -lpqxx

clean:
	rm -f insbench