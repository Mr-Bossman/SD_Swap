PROGRAM = sdswap

CFLAGS :=	-Wall \
		-Wextra \
		-Wpedantic \
		-Werror \
		-Wno-zero-length-array \
		-std=c99

LDFLAGS :=

HOST := $(shell $(CC) -dumpmachine)
C_SOURCES = src/sdswap.c

PREFIX ?= /usr/local

VARIANT := $(PROGRAM)

ifneq (, $(shell pkg-config --version 2>/dev/null))
	CFLAGS += $(shell pkg-config --cflags libusbgetdev)
	LDFLAGS += $(shell pkg-config --libs libusbgetdev)
else
	CFLAGS += -I/usr/include/libusb-1.0 -I/usr/local/include/libusbgetdev
	LDFLAGS += -lusb-1.0 -lusbgetdev
endif

.PHONY: all clean debug install uninstall portable

all: $(PROGRAM)

debug: CFLAGS += -g
debug: all

portable:
	@$(MAKE) VARIANT=$(PROGRAM)_c

src/$(PROGRAM)_c: $(C_SOURCES)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(PROGRAM): src/$(VARIANT)
	cp src/$(VARIANT) $@

install: all
	install -m 755 $(PROGRAM) $(PREFIX)/bin

uninstall:
	-rm -f $(PREFIX)/bin/$(PROGRAM)

clean:
	-rm -f src/$(PROGRAM)_c
	-rm -f $(PROGRAM)
