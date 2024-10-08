PWD=$(shell pwd)
BUILD=$(PWD)/build
MAN_DIR=$(BUILD)/share/man/man1
VERSION='"$(shell git describe --tags --dirty)"'

CC=gcc
CFLAGS=-I$(BUILD)/include -std=c99 -DVIRSH_SS_VERSION=$(VERSION)
CFLAGS_RELEASE=-O2
CFLAGS_DEBUG=-Wall -Wextra -g -DDEBUG=1
LDFLAGS=-L$(BUILD)/lib -lutils

BUILD_DIRS=$(BUILD) \
		   $(BUILD)/lib \
		   $(BUILD)/bin

LIBUTILS_CONFIG=$(PWD)/lib/libutils.conf
LIBUTILS=$(BUILD)/lib/libutils.a

TARGET=$(BUILD)/bin/virsh-ss
MAN_PAGE=$(PWD)/doc/virsh-ss.man
PREFIX?=/usr

.PHONY: release
release: TASK=release
release: CFLAGS+=$(CFLAGS_RELEASE)
release: build

.PHONY: debug
debug: TASK=debug
debug: CFLAGS+=$(CFLAGS_DEBUG)
debug: build

# make_build_dir(dir_name)
define make_build_dir
$(1):
	mkdir -p $$@
endef

# Build directory rules
$(foreach build_dir, $(BUILD_DIRS), \
	$(eval $(call make_build_dir,$(build_dir))))

.PHONY: install
install:
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/man/man1
	cp $(TARGET) $(PREFIX)/bin
	gzip -c $(MAN_PAGE) > $(PREFIX)/share/man/man1/virsh-ss.1.gz

.PHONY: build
build: $(BUILD_DIRS) $(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: $(LIBUTILS)
$(LIBUTILS): lib/c-utils
	$</version.sh nanorl 1.2.1
	$(MAKE) -C $< $(TASK) \
		CONFIG_PATH=$(LIBUTILS_CONFIG) \
		BUILD=$(BUILD)

$(TARGET): src/main.c src/charmap.h $(LIBUTILS)
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
