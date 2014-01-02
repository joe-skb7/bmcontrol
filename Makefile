APP = bmcontrol
CXXFLAGS = --std=c++11 -Wall -pedantic
LDFLAGS = -lusb
OBJS = main.o

ifeq ($(OS),Windows_NT)
	RM = del /Q
	BIN = $(APP).exe
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
