OPTS = -Wall -g 
OBJS = *.o

wish: wish.o common.o exec.o
	gcc -o wish common.o exec.o wish.o

wish.o: wish.c common.o exec.o 
	gcc -o wish.o $(OPTS) -c wish.c 

exec.o: common.o exec.c 
	gcc -o exec.o $(OPTS) -c exec.c

common.o: common.c
	gcc -o common.o $(OPTS) -c common.c

all: wish

clean: 
	rm wish $(OBJS)


