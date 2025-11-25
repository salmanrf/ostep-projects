OPTS = -Wall -g
OBJS = *.o
TEMPS = *.txt *.z

wzip: wzip.o
	gcc -o wzip wzip.o

wzip.o: wzip.c
	gcc -o wzip.o $(OPTS) -c wzip.c

wunzip: wunzip.o
	gcc -o wunzip wunzip.o

wunzip.o: wunzip.c
	gcc -o wunzip.o $(OPTS) -c wunzip.c

all: wzip wunzip

clean: 
	rm wzip wunzip $(TEMPS) $(OBJS)


