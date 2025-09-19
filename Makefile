CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -Wextra
LIBS = -lz
TARGET = compresor
SRC = src/parallel_compression.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET) *.bin temp_*.bin

test: $(TARGET)
	./$(TARGET)

.PHONY: clean test
