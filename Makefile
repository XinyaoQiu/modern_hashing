# CXX = g++
# CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# TEST_SRCS = iceberg_unittest.cpp \
#             cuckoo_unittest.cpp \
#             cuckoo_stress_test.cpp

# TEST_OBJS = $(TEST_SRCS:.cpp=.o)
# TEST_TARGETS = $(TEST_SRCS:.cpp=)

# all: $(TEST_TARGETS)

# $(TEST_TARGETS): %: %.o
# 	$(CXX) $(CXXFLAGS) -o $@ $<

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $<

# .PHONY: run
# run:
# 	@echo "Available tests:"; \
# 	for t in $(TEST_TARGETS); do echo "  make run-$$t"; done

# $(foreach t,$(TEST_TARGETS),\
#   $(eval .PHONY: run-$(t))\
#   $(eval run-$(t): $(t) ; ./$(t))\
# )

# clean:
# 	rm -f $(TEST_TARGETS) $(TEST_OBJS)


# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude

# Directories
SRC_DIR := src
TEST_DIR := test
BIN_DIR := bin

# Find all test .cpp files and generate target names
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
TEST_NAMES := $(notdir $(basename $(TEST_SOURCES)))
TEST_BINARIES := $(addprefix $(BIN_DIR)/, $(TEST_NAMES))

# Default: build all test binaries
all: $(TEST_BINARIES)

# Rule to compile each test binary
$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(wildcard $(SRC_DIR)/*.cpp)

# Ensure bin/ exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Shortcut: make test target by name (e.g. make cuckoo_unittest)
$(TEST_NAMES): %: $(BIN_DIR)/%
	@echo "Running $@"
	./$<

# Clean all binaries
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean $(TEST_NAMES)


