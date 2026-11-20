CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2 -Iincluye

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
TOOL_DIR  := herramientas

SOURCES   := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS   := $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TARGET    := $(BIN_DIR)/gestor_buffer
TSV_TOOL  := $(BIN_DIR)/tsv_a_bin

.PHONY: all clean run-tsv run

all: $(TARGET) $(TSV_TOOL)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TSV_TOOL): $(TOOL_DIR)/tsv_a_bin.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run-tsv: $(TSV_TOOL)
	./$(TSV_TOOL) datos/ratings.tsv datos/ratings.bin

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
