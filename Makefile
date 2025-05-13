# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude -g

# Directories
TEST_DIR := test
EVAL_DIR := evaluation
BIN_DIR := bin
OUTPUT_DIR := output

# Automatically find all test and evaluation source files
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
EVAL_SOURCES := $(wildcard $(EVAL_DIR)/*.cpp)

# Extract base names
TEST_NAMES := $(notdir $(basename $(TEST_SOURCES)))
EVAL_NAMES := $(notdir $(basename $(EVAL_SOURCES)))

# Map to output binaries
TEST_BINARIES := $(addprefix $(BIN_DIR)/, $(TEST_NAMES))
EVAL_BINARIES := $(addprefix $(BIN_DIR)/, $(EVAL_NAMES))

# Default: build all test and evaluation binaries
all: test eval

test: $(TEST_BINARIES)
eval: $(EVAL_BINARIES)

# Ensure bin/ exists before building
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Ensure output/ exists before building
$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

# Build rule for test binaries
$(BIN_DIR)/%: $(TEST_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build rule for eval binaries
$(BIN_DIR)/%: $(EVAL_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Shortcut: make target name (e.g. make cuckoo_unittest or baseline_eval)
$(TEST_NAMES) $(EVAL_NAMES): %: $(BIN_DIR)/%
	@echo "Running $@"
	./$<

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean test eval $(TEST_NAMES) $(EVAL_NAMES)
