TYPE = so
TARGET = libopenai_api.$(TYPE)
TEST_BIN = openai_api.bin

CC = g++
OBJS = $(wildcard *.cpp)
LIBS = -pthread -lcurl -lssl -lcrypto -lz 
FLAGS = -shared -fPIC -g


$(TEST_BIN): $(TARGET)
	$(CC) $^ -o $@ $(LIBS)

$(TARGET): $(OBJS:.cpp=.o)
	$(CC) $^ -o $@ $(FLAGS)

%.o:%.cpp
	$(CC) $^ -c -o $@ $(FLAGS)

test:
	LD_LIBRARY_PATH=$(shell pwd):$$LD_LIBRARY_PATH ./$(TEST_BIN)

memcheck:
	LD_LIBRARY_PATH=$(shell pwd):$$LD_LIBRARY_PATH valgrind --leak-check=full --show-reachable=yes --trace-children=yes ./$(TEST_BIN)

clean:
	rm -f $(OBJS:.cpp=.o) $(TARGET) $(TEST_BIN)

PHONY: clean test
