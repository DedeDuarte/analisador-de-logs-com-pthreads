CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm

SRC = sensor_analyzer_seq.c
TARGET = a.out

all:
	@$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	@rm -f $(TARGET)