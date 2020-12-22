.PHONY: all clean

all: size2bin

CFLAGS = -Os -s -Wall -Wextra

size2bin: size2bin.o
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f size2bin *.o
