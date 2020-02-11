#include "everything.h"

void initAB(appendBuffer *ab, int size) {
    ab->size = size;
    ab->buffer = malloc(sizeof(char) * ab->size);
    ab->len = 0;
}

void freeAB(appendBuffer *ab) {
    free(ab->buffer);
    free(ab);
}

void appendCharAB(appendBuffer *ab, char c) {
    // if there is not enough of space, realloc
    if (ab->len + 2 >= ab->size)        
        expandAB(ab);
    ab->buffer[ab->len++] = c;
}

void appendAB(appendBuffer *ab, char* buffer, int len) {
    if (ab->len + len >= ab->size)
        expandAB(ab);
    memcpy(&ab->buffer[ab->len], buffer, len);
    ab->len += len;
}

void refreshAB(appendBuffer *ab) {
    ab->len = 0;
}

void expandAB(appendBuffer *ab) {
    int newsize = ab->size * 2;
    ab->buffer = realloc(ab->buffer, newsize * sizeof(char));
    if (ab->buffer == NULL) {
        perror("failed to expand appendBuffer");
        exit(1);
    }
    ab->size = newsize;
}
