CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11
TARGET = pipeline
SRCS = main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf output/main

.PHONY: all run clean
