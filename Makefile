PWD=$(shell pwd)
BUILD=$(PWD)/build

CC=gcc
CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-Wall -Wextra -g -DDEBUG=1
LDFLAGS=-L$(BUILD)/lib -lutils -lreadline

LIBUTILS_CONFIG=$(PWD)/lib/libutils.conf
LIBUTILS=$(BUILD)/lib/libutils.a

TARGET_RELEASE=$(BUILD)/virsh-ss
TARGET_DEBUG=$(BUILD)/virsh-ss-debug

.PHONY: all
all: release debug

.PHONY: release
release: $(BUILD) $(LIBUTILS) $(TARGET_RELEASE)

.PHONY: debug
debug: $(BUILD) $(LIBUTILS) $(TARGET_DEBUG)

.PHONY: clean
clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(LIBUTILS): lib/c-utils
	$(MAKE) -C $< \
		CONFIG_PATH=$(LIBUTILS_CONFIG) \
		BUILD=$(BUILD)

$(TARGET_RELEASE): main.c charmap.h
	$(CC) $(CFLAGS_RELEASE) -o $@ $< $(LDFLAGS)

$(TARGET_DEBUG): main.c charmap.h
	$(CC) $(CFLAGS_DEBUG) -o $@ $< $(LDFLAGS)

# Formatting
FORMAT=clang-format
FORMAT_CHECK_FLAGS=--dry-run --Werror
FORMAT_FIX_FLAGS=-i

FORMAT_FILES=main.c charmap.h

.PHONY: checkformat
checkformat:
	$(FORMAT) $(FORMAT_CHECK_FLAGS) $(FORMAT_FILES)

.PHONY: format
format:
	$(FORMAT) $(FORMAT_FIX_FLAGS) $(FORMAT_FILES)
