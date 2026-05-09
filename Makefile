CC=gcc

CFLAG=-Wall -g -Iinclude

SRC=$(wildcard src/*.c)

TARGET=mini-monitor

all:
	$(CC) $(CFLAG) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)