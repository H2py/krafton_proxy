# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

# Target executable
TARGET = thread_example

all: $(TARGET)

$(TARGET): main.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean


