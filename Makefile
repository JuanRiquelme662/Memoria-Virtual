CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread
TARGET  = vm
SRCS    = vm.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -pthread

clean:
	rm -f $(TARGET) correct.txt *.o

.PHONY: all clean