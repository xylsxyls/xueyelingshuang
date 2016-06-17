#Makefile
CC = g++ 
CFLAGS = -g -DDEBUG
LIBS = -lpthread
INCLUDE = -l/usr/src
OBJ = main
OBJS = main.o cstring.o fixalloc.o
all:clean install

install:$(OBJS) main

main:
	$(CC)  $(OBJS) $(LIBS) -o  $@
$(OBJS):%.o:%.cpp
	$(CC) -c $(CFLAGS) $<  -o $@

.PHONY:clean
clean:
	-rm *.o
	-rm *.~
	-rm $(OBJ)


