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
EVAL_DIR := evaluation
BIN_DIR := bin

# Automatically find all test and evaluation files
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
EVAL_SOURCES := $(wildcard $(EVAL_DIR)/*.cpp)

TEST_NAMES := $(notdir $(basename $(TEST_SOURCES)))
EVAL_NAMES := $(notdir $(basename $(EVAL_SOURCES)))

TEST_BINARIES := $(addprefix $(BIN_DIR)/, $(TEST_NAMES))
EVAL_BINARIES := $(addprefix $(BIN_DIR)/, $(EVAL_NAMES))

# Group targets
all: test eval

test: $(TEST_BINARIES)
eval: $(EVAL_BINARIES)

# Ensure bin/ exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build rule for tests
$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(wildcard $(SRC_DIR)/*.cpp)

# Build rule for evals
$(BIN_DIR)/%: $(EVAL_DIR)/%.cpp $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< $(wildcard $(SRC_DIR)/*.cpp)

# Shortcut: make target_name to run test or eval
$(TEST_NAMES) $(EVAL_NAMES): %: $(BIN_DIR)/%
	@echo "Running $@"
	./$<

# Clean
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean test eval $(TEST_NAMES) $(EVAL_NAMES)


