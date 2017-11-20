CC := g++
CFLAGS := -Wall -g --std=c++11 -lpthread

OBJECTS := tinwebserv.o ssocklib.o

twebserv: $(OBJECTS)
	$(CC) $(CFLAGS) -o twebserv $(OBJECTS)
tinwebserv.o: tinwebserv.cpp
	$(CC) $(CFLAGS) -c $<
ssocklib.o: ssocklib.cpp
	$(CC) $(CFLAGS) -c $<

.PHONY:clean
clean:
	-rm -f twebserv *.o
