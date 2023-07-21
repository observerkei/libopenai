TYPE = app
TARGET = openai_api.$(TYPE)

CC = g++
OBJS = $(wildcard *.cpp)
LIBS = -g  -pthread -lcurl -lssl -lcrypto -lz  

$(TARGET): $(OBJS:.cpp=.o)
	$(CC) $^ -o $@ $(LIBS)

%.o:%.cpp
	$(CC) $^ -c -o $@

clean:
	rm -f $(OBJS:.cpp=.o) $(TARGET)

PHONY: clean
