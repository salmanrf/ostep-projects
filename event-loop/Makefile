CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS =
ifeq ($(shell uname),Linux)
LDFLAGS += -lrt
endif

# Server
SERVER_TARGET = server
SERVER_SRCS = server.c server_handler.c state.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

.PHONY: all clean

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

server.o: server.c common.h server.h state.h
server_handler.o: server_handler.c server.h state.h common.h
state.o: state.c state.h common.h
state_manager.o: state_manager.c state.h common.h

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET)
