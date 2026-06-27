#include "common.h"

typedef enum {
    WAITING_INPUT = 1,
    WAITING_READ = 2,
    WAITING_WRITE = 3
} EREQ_STATUS;

typedef enum {
    STATE_CLIENT = 0,
    STATE_ASSET = 1
} EREQ_STATE_TYPE;

typedef struct REQ_STATE {
    int conn_fd;
    struct REQ_STATE *parent;
    EREQ_STATE_TYPE type;
    EREQ_STATUS status;
    struct aiocb *aioctl;
} REQ_STATE;

typedef struct {
    int size;
    REQ_STATE **store;
} STATE_STORE;

STATE_STORE *init_state_store(int size);

REQ_STATE *new_state(int fd, EREQ_STATE_TYPE type);

REQ_STATE *get_state(STATE_STORE *ss, int key);

void set_state(STATE_STORE *ss, int key, REQ_STATE *new_state);
