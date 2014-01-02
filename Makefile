APP = bmcontrol
CFLAGS = -Wall -pedantic -O2 -s
LDFLAGS = -lusb
OBJS = main.o

ifeq ($(OS),Windows_NT)
	RM = del /Q
	BIN = $(APP).exe
	OBJS += nanosleep_win32.o
else
	ifeq ($(shell uname -s), Linux)
		RM = rm -f
		BIN = $(APP)
	endif
endif

default: $(BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o $(BIN)

.PHONY: default clean
