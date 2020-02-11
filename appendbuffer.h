#ifndef APPENDBUFFER
#define APPENDBUFFER

#include <stdlib.h>
#include <string.h>

#define ABUF_INIT {NULL, 0}

typedef struct abuf {
    char *buffer;
    int size;
    int len;
} appendBuffer;

void initAB(appendBuffer*, int);
void freeAB(appendBuffer*);
void appendCharAB(appendBuffer*, char);
void appendAB(appendBuffer*, char*, int);
void refreshAB(appendBuffer*);
void expandAB(appendBuffer*);

#endif