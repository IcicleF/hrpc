.PHONY: all clean

TESTS = $(patsubst %.cpp,%,$(wildcard *.cpp))
CXXFLAGS = -std=c++20 -g -Wall -Wextra -I../include
LDFLAGS = 

all: $(TESTS)

%: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TESTS)
