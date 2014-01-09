APP = bmcontrol
CFLAGS = -Wall -pedantic -O2 -s -Iinclude
LDFLAGS = -lusb
OBJS = src/main.o

ifeq ($(OS),Windows_NT)
	RM = del /Q
	CP = copy /Y
	MKDIR = mkdir
	BIN = $(APP).exe
	OBJS += src/win32/nanosleep_win32.o
	PREFIX ?= "C:\Program Files\BMControl"
	PREFIX_BIN = $(PREFIX)
else
	ifneq ($(shell uname -s), Linux)
$(warning *** (warning) Currently only Linux and Windows build is supported. \
Trying to use the same build configuration as on Linux)
	endif

	CFLAGS += -std=c99 -D_POSIX_C_SOURCE=199309L
	RM = rm -f
	CP = install -m 0755
	MKDIR = mkdir -p
	BIN = $(APP)
	PREFIX ?= /usr/local
	PREFIX_BIN = $(PREFIX)/bin
endif

default: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(BIN) $(OBJS)

install:
	$(MKDIR) $(PREFIX_BIN)
	$(CP) $(BIN) $(PREFIX_BIN)

.PHONY: default clean
