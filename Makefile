CC=gcc
CFLAGS=-Wall -Wextra -g
LDFLAGS=
BUILDDIR=build
OUT=$(BUILDDIR)/virsh-ss

.PHONY=all
all: $(BUILDDIR) $(OUT)

.PHONY=clean
clean:
	rm -r $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(OUT): main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
