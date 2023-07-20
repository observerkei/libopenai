TYPE = app
TARGET = openai_api.$(TYPE)

CC = g++
OBJS = $(wildcard *.cpp)

TARGET: $(OBJS:.cpp=.o)
	$(CC) $^ -o $@

%.o:%.cpp
	$(CC) $^ -o $@


clean:
	rm -f *.o $(TARGET)

PHONY: clean
