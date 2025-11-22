CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LDFLAGS = -lpthread

TARGET = dp_app

SRCS = main.cpp

all: $(TARGET)

$(TARGET): $(SRCS) Database.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

distclean: clean
	rm -f *.db *.csv

.PHONY: all clean distclean