APP = bmcontrol
CXXFLAGS = --std=c++11 -Wall -pedantic
LDFLAGS = -lusb
OBJS = main.o

default: $(APP)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(APP): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(APP)

.PHONY: default $(APP) clean
