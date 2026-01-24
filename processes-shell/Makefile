OPTS = -Wall -g 
OBJS = wish.o common.o exec.o

wish: $(OBJS)
	gcc $(OPTS) -o wish $(OBJS)

wish.o: wish.c
	gcc $(OPTS) -c wish.c

exec.o: exec.c
	gcc $(OPTS) -c exec.c

common.o: common.c
	gcc $(OPTS) -c common.c

all: wish

clean: 
	rm -f wish $(OBJS)
