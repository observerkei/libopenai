TYPE = so
TARGET = libopenai_api.$(TYPE)
TEST_APP = openai_api.app

CC = g++
SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,%.o,$(filter-out main.cpp,$(SRCS)))
LIBS = -pthread -lcurl -lssl -lcrypto -lz 
FLAGS = -shared -fPIC -g

$(TEST_APP): $(TARGET) main.o
	$(CC) $^ -o $@ $(LIBS)

$(TARGET): $(OBJS:.cpp=.o)
	$(CC) $^ -o $@ $(FLAGS)

main.o:main.cpp
	$(CC) $^ -c -o $@ $(FLAGS)

%.o:%.cpp
	$(CC) $^ -c -o $@ $(FLAGS)

test:
	LD_LIBRARY_PATH=$(shell pwd):$$LD_LIBRARY_PATH ./$(TEST_APP)

memcheck:
	LD_LIBRARY_PATH=$(shell pwd):$$LD_LIBRARY_PATH valgrind --leak-check=full --show-reachable=yes --trace-children=yes ./$(TEST_APP)

clean:
	rm -f $(OBJS:.cpp=.o) $(TARGET) $(TEST_APP) main.o

PHONY: clean test
