CC=gcc
CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-Wall -Wextra -g -DDEBUG=1
LDFLAGS=-lreadline

BUILD=build
TARGET_RELEASE=$(BUILD)/virsh-ss
TARGET_DEBUG=$(BUILD)/virsh-ss-debug

.PHONY: all
all: release debug

.PHONY: release
release: $(BUILD) $(TARGET_RELEASE)

.PHONY: debug
debug: $(BUILD) $(TARGET_DEBUG)

.PHONY: clean
clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(TARGET_RELEASE): main.c charmap.h
	$(CC) $(CFLAGS_RELEASE) -o $@ $< $(LDFLAGS)

$(TARGET_DEBUG): main.c charmap.h
	$(CC) $(CFLAGS_DEBUG) -o $@ $< $(LDFLAGS)
