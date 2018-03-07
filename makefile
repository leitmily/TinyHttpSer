CC := g++
CFLAGS := -Wall -g --std=c++11 -lpthread

OBJECTS := twebserv.o socklib.o handle.o common.o
TARGET := twebserv


$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o twebserv $(OBJECTS)
twebserv.o: twebserv.cpp handle.h socklib.h
	$(CC) $(CFLAGS) -c $<
socklib.o: socklib.cpp socklib.h
	$(CC) $(CFLAGS) -c $<
handle.o: handle.cpp handle.h common.h
	$(CC) $(CFLAGS) -c $<
common.o: common.cpp common.h handle.h
	$(CC) $(CFLAGS) -c $<

.PHONY:clean
clean:
	-rm -f $(TARGET) *.o
