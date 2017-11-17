CC := g++
CFLAGS := -Wall -g --std=c++11 -lpthread

OBJECTS := twebserv.o socklib.o

twebserv: $(OBJECTS)
	$(CC) $(CFLAGS) -o twebserv $(OBJECTS)
twebserv.o: twebserv.cpp
	$(CC) $(CFLAGS) -c $<
socklib.o: socklib.cpp
	$(CC) $(CFLAGS) -c $<

.PHONY:clean
clean:
	-rm -f twebserv *.o
#test git
