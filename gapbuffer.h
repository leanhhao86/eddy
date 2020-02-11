#ifndef GAPBUFFER
#define GAPBUFFER
/***
    * Gapbuffer data structure
    * Basically a continuous buffer with a gap in the middle
 ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

typedef struct gapBuffer {
    char *buffer;
    int size;   
    int presize;        /* position past the last character of pre-part */
    int postsize;
} gapBuffer;


void initGB(gapBuffer*, int);
void freeGB(gapBuffer*);
void expandGB(gapBuffer*);
void moveCursorForwardGB(gapBuffer*);
void moveCursorBackwardGB(gapBuffer*);
void moveNCursorGB(gapBuffer*, int);
void moveCursorToIdxGB(gapBuffer*, int);
void insertCharGB(gapBuffer*, char);
void deleteCharGB(gapBuffer*);
void insertStringGB(gapBuffer*, char*, int);
int getLengthGB(gapBuffer*);
int mapGB(char*, gapBuffer*, int);
int getCursorPosGB(gapBuffer*);
int getCursorNextPosGB(gapBuffer*);	
void shiftRightGB(gapBuffer*);
void shiftLeftGB(gapBuffer*);
void displayStringGB(gapBuffer*);
int isCursorLeftGB(gapBuffer *);
int isCursorRightGB(gapBuffer *);
void displayGB(gapBuffer *);
gapBuffer* combineGB(gapBuffer*, gapBuffer*);
gapBuffer* splitGB(gapBuffer *);
#endif
