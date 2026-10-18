CC = gcc
OPTS = -g -Wall -Werror -pthread -O
DEBUG_OPTS = $(filter-out -O,$(OPTS)) -O0

# Files with a main() (test applications); every other .c file is part of the
# mapreduce library and gets linked into each application
MAINS = wordcount.c
SRCS = $(wildcard *.c)
LIB_SRCS = $(filter-out $(MAINS),$(SRCS))
LIB_OBJS = $(LIB_SRCS:.c=.o)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
DEBUG_LIB_OBJS = $(LIB_SRCS:.c=.debug.o)
DEBUG_OBJS = $(SRCS:.c=.debug.o)
DEBUG_DEPS = $(SRCS:.c=.debug.d)

BINS = $(MAINS:.c=)
DEBUG_BINS = $(MAINS:.c=-debug)

all: $(BINS)

# Non-optimized build; separate objects/binaries so it never mixes with the -O ones
debug: $(DEBUG_BINS)

$(DEBUG_BINS): %-debug: %.debug.o $(DEBUG_LIB_OBJS)
	$(CC) -o $@ $(DEBUG_OPTS) $^

$(BINS): %: %.o $(LIB_OBJS)
	$(CC) -o $@ $(OPTS) $^

# -MMD generates .d files so header changes trigger rebuilds automatically
%.o: %.c
	$(CC) -o $@ $(OPTS) -MMD -MP -c $<

%.debug.o: %.c
	$(CC) -o $@ $(DEBUG_OPTS) -MMD -MP -c $<

-include $(DEPS) $(DEBUG_DEPS)

clean:
	rm -f $(BINS) $(DEBUG_BINS) $(OBJS) $(DEPS) $(DEBUG_OBJS) $(DEBUG_DEPS)

.PHONY: all debug clean
