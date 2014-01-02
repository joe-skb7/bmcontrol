APP = bmcontrol
CXXFLAGS = --std=c++11 -Wall -pedantic -O2 -s
LDFLAGS = -lusb
OBJS = main.o

ifeq ($(OS),Windows_NT)
	RM = del /Q
	BIN = $(APP).exe
	CXXFLAGS += -Wno-pedantic-ms-format
	OBJS += nanosleep_win32.o
else
	ifeq ($(shell uname -s), Linux)
		RM = rm -f
		BIN = $(APP)
	endif
endif

default: $(BIN)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(BIN): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o $(BIN)

.PHONY: default clean
