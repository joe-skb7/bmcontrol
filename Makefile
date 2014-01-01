CC = g++

all: bmcontrol

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -I. -I/usr/include -o $@ $^ -lusb


bmcontrol: main.o
	$(CC) -I/usr/include -L/usr/lib -o $@ $^ -lusb


clean: 
	rm -f *.o bmcontrol