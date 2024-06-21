PWD=$(shell pwd)
BUILD=$(PWD)/build
MAN_DIR=$(BUILD)/share/man/man1

CC=gcc
CFLAGS=-I$(BUILD)/include -std=c99
CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-Wall -Wextra -g -DDEBUG=1
LDFLAGS=-L$(BUILD)/lib -lutils

LIBUTILS_CONFIG=$(PWD)/lib/libutils.conf
LIBUTILS=$(BUILD)/lib/libutils.a

TARGET=$(BUILD)/bin/virsh-ss
MAN_TARGET=$(MAN_DIR)/virsh-ss.1.gz
PREFIX?=/usr

.PHONY: release
release: TASK=release
release: CFLAGS+=$(CFLAGS_RELEASE)
release: build

.PHONY: debug
debug: TASK=debug
debug: CFLAGS+=$(CFLAGS_DEBUG)
debug: build

.PHONY: install
install: $(BUILD)/share/man/man1/virsh-ss.1.gz
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	cp $(TARGET) $(PREFIX)/bin
	cp $(MAN_TARGET) $(PREFIX)/share/man/man1

.PHONY: build
build: $(BUILD) $(LIBUTILS) $(TARGET)

$(MAN_TARGET): $(PWD)/doc/virsh-ss.man
	mkdir -p $(MAN_DIR)
	gzip -c $< > $(MAN_TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(LIBUTILS): lib/c-utils
	$(MAKE) -C $< $(TASK) \
		CONFIG_PATH=$(LIBUTILS_CONFIG) \
		BUILD=$(BUILD)

$(TARGET): src/main.c src/charmap.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Formatting
FORMAT=clang-format
FORMAT_CHECK_FLAGS=--dry-run --Werror
FORMAT_FIX_FLAGS=-i

FORMAT_FILES=src/main.c src/charmap.h

.PHONY: checkformat
checkformat:
	$(FORMAT) $(FORMAT_CHECK_FLAGS) $(FORMAT_FILES)

.PHONY: format
format:
	$(FORMAT) $(FORMAT_FIX_FLAGS) $(FORMAT_FILES)
