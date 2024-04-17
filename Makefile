CC=gcc
CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-Wall -Wextra -g -DDEBUG=1
LDFLAGS=-lreadline
OUT_RELEASE=virsh-ss
OUT_DEBUG=virsh-ss-debug

.PHONY=release
release: $(OUT_RELEASE)

.PHONY=debug
debug: $(OUT_DEBUG)

.PHONY=clean
clean:
	rm -f $(OUT_RELEASE) $(OUT_DEBUG)

$(OUT_RELEASE): main.c charmap.h
	$(CC) $(CFLAGS_RELEASE) -o $@ $< $(LDFLAGS)

$(OUT_DEBUG): main.c charmap.h
	$(CC) $(CFLAGS_DEBUG) -o $@ $< $(LDFLAGS)
