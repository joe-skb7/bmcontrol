APP = bmcontrol
CFLAGS = -Wall -pedantic -O2 -s
LDFLAGS = -lusb
OBJS = main.o

ifeq ($(OS),Windows_NT)
	RM = del /Q
	CP = copy /Y
	MKDIR = mkdir
	BIN = $(APP).exe
	OBJS += nanosleep_win32.o
	PREFIX ?= "C:\Program Files\BMControl"
	PREFIX_BIN = $(PREFIX)
else
	ifeq ($(shell uname -s), Linux)
		CFLAGS += -std=c99 -D_POSIX_C_SOURCE=199309L
		RM = rm -f
		CP = install -m 0755
		MKDIR = mkdir -p
		BIN = $(APP)
		PREFIX ?= /usr/local
		PREFIX_BIN = $(PREFIX)/bin
	endif
endif

default: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o $(BIN)

install:
	$(MKDIR) $(PREFIX_BIN)
	$(CP) $(BIN) $(PREFIX_BIN)

.PHONY: default clean
