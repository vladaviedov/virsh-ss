CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=
OUT=virsh-ss

.PHONY=all
all: $(OUT)

.PHONY=clean
clean:
	rm $(OUT)

$(OUT): main.c charmap.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
