CXX=g++
CXXFLAGS=-g -Wall -O0 -pthread 

all: dtm_recovery

dtm_recovery: dtm_recovery.cpp
	$(CXX) $(CXXFLAGS) -o dtm_recovery dtm_recovery.cpp -lpqxx

clean:
	rm -f dtm_recovery
