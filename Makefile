CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

ICEBERG_SRCS = iceberg_unittest.cpp
CUCKOO_SRCS = cuckoo_unittest.cpp

ICEBERG_OBJS = $(ICEBERG_SRCS:.cpp=.o)
CUCKOO_OBJS = $(CUCKOO_SRCS:.cpp=.o)

ICEBERG_TARGET = test_iceberg
CUCKOO_TARGET = test_cuckoo

all: $(ICEBERG_TARGET) $(CUCKOO_TARGET)

$(ICEBERG_TARGET): $(ICEBERG_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(CUCKOO_TARGET): $(CUCKOO_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Run targets
run-iceberg: $(ICEBERG_TARGET)
	./$(ICEBERG_TARGET)

run-cuckoo: $(CUCKOO_TARGET)
	./$(CUCKOO_TARGET)

run:
	@echo "To run the tests, use:"
	@echo "  make run-iceberg   # to run IcebergHash tests"
	@echo "  make run-cuckoo     # to run CuckooHash tests"

clean:
	rm -f $(ICEBERG_TARGET) $(CUCKOO_TARGET) $(ICEBERG_OBJS) $(CUCKOO_OBJS)