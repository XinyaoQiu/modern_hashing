CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

TEST_SRCS = iceberg_unittest.cpp \
            cuckoo_unittest.cpp \
            cuckoo_stress_test.cpp

TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_TARGETS = $(TEST_SRCS:.cpp=)

all: $(TEST_TARGETS)

$(TEST_TARGETS): %: %.o
	$(CXX) $(CXXFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: run
run:
	@echo "Available tests:"; \
	for t in $(TEST_TARGETS); do echo "  make run-$$t"; done

$(foreach t,$(TEST_TARGETS),\
  $(eval .PHONY: run-$(t))\
  $(eval run-$(t): $(t) ; ./$(t))\
)

clean:
	rm -f $(TEST_TARGETS) $(TEST_OBJS)
