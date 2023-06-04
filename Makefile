CC = gcc
CFLAGS = -Wall -Wextra -pthread

SRC = main.c buffers.c actors.c
OBJ = $(SRC:.c=.o)

TARGET = ex3.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)



