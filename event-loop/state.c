#include "state.h"

STATE_STORE *init_state_store(int size) {
    STATE_STORE *ss = malloc(sizeof(STATE_STORE));
    REQ_STATE **store = (REQ_STATE **) malloc(sizeof(REQ_STATE *) * size);
   
    ss->size = size;
    ss->store = store;

    for(int i = 0 ; i < size; i++) {
       ss->store[0] = NULL; 
    }

    return ss;
}

REQ_STATE *new_state(int fd, EREQ_STATE_TYPE type) {
    REQ_STATE *state = (REQ_STATE *) malloc(sizeof(REQ_STATE));
    state->type = type; 
    state->conn_fd = fd;
    state->status = WAITING_INPUT;

    return state;
}

REQ_STATE *get_state(STATE_STORE *ss, int key) {
    if(ss == NULL) {
        return NULL;
    }

    int index = key % ss->size;
    REQ_STATE *state = ss->store[index];

    return state;
}

void set_state(STATE_STORE *ss, int key, REQ_STATE *new_state) {
    if(ss == NULL) {
        return;
    }

    int index = key % ss->size;
    ss->store[index] = new_state;
}
