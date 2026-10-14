CC = gcc
OPTS = -g -Wall -Werror -pthread -O
DEBUG_OPTS = $(filter-out -O,$(OPTS)) -O0
TEMPS = *.txt *.z

# Files with a main(); every other .c file is a shared module linked into pzip
MAINS = pzip.c punzip.c
SRCS = $(wildcard *.c)
LIB_SRCS = $(filter-out $(MAINS),$(SRCS))
LIB_OBJS = $(LIB_SRCS:.c=.o)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
DEBUG_LIB_OBJS = $(LIB_SRCS:.c=.debug.o)
DEBUG_OBJS = $(SRCS:.c=.debug.o)
DEBUG_DEPS = $(SRCS:.c=.debug.d)

all: pzip punzip

# Non-optimized build; separate objects/binaries so it never mixes with the -O ones
debug: pzip-debug punzip-debug

pzip-debug: pzip.debug.o $(DEBUG_LIB_OBJS)
	$(CC) -o $@ $(DEBUG_OPTS) $^

punzip-debug: punzip.debug.o
	$(CC) -o $@ $(DEBUG_OPTS) $^

pzip: pzip.o $(LIB_OBJS)
	$(CC) -o $@ $(OPTS) $^

punzip: punzip.o
	$(CC) -o $@ $(OPTS) $^

# -MMD generates .d files so header changes trigger rebuilds automatically
%.o: %.c
	$(CC) -o $@ $(OPTS) -MMD -MP -c $<

%.debug.o: %.c
	$(CC) -o $@ $(DEBUG_OPTS) -MMD -MP -c $<

-include $(DEPS) $(DEBUG_DEPS)

clean:
	rm -f pzip punzip pzip-debug punzip-debug $(TEMPS) $(OBJS) $(DEPS) $(DEBUG_OBJS) $(DEBUG_DEPS)

.PHONY: all debug clean
