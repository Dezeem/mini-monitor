CC=gcc

CFLAG=-Wall -g -Iinclude -pthread

SRC=$(wildcard src/*.c)

TARGET=./build/mini-monitor

all:
	$(CC) $(CFLAG) $(SRC) -o $(TARGET)

run:
	$(TARGET)

clean:
	rm -f $(TARGET)