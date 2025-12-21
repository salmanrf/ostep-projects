OPTS = -Wall -g
OBJS = *.o
TEMPS = *.txt *.z

reverse: reverse.o
	gcc -o reverse reverse.o

reverse.o: reverse.c
	gcc -o reverse.o $(OPTS) -c reverse.c

all: reverse

clean: 
	rm reverse $(TEMPS) $(OBJS)


