CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCS = iceberg_unittest.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET = test_iceberg

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
