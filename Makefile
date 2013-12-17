SRC_DIR = src
SRC_FILES = main.cpp
TARGET = demo

CXX=g++
#CXXFLAGS = -g -pg -Wall -Isrc/  -std=c++11
CXXFLAGS = -Wall -Isrc/ -s -fdata-sections -std=c++11
# CXXFLAGS = -O3 -Wall -Isrc/ -std=c++11
LDFLAGS += -Llib -lsfml-system -lsfml-window -lsfml-audio -lGL
###########################################################

CXX_FILES = $(SRC_FILES:%=$(SRC_DIR)/%)
O_FILES = $(CXX_FILES:.cpp=.o)

all: $(TARGET)

strip: $(TARGET)
	strip --strip-all --remove-section=.comment --remove-section=.note demo

$(TARGET): $(O_FILES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)


clean:
	@rm -vf $(O_FILES)

distclean: clean
	@rm -vf $(TARGET)

run: $(TARGET)
	LD_LIBRARY_PATH=lib/ ./demo

debug:
	LD_LIBRARY_PATH=lib/ gdb ./demo
