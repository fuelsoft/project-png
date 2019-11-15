BASE_FILE = png.cpp
INC_FILES = Chunk.cpp
HEADER_FILES = Chunk.hpp
OUTPUT = png
COMPILER = clang++
OPT_LEVEL = -O2
STD = -std=c++11

make: $(BASE_FILE) $(INC_FILES) $(HEADER_FILES)
	$(COMPILER) $(BASE_FILE) $(INC_FILES) -o $(OUTPUT) -Wall $(OPT_LEVEL) $(STD)
