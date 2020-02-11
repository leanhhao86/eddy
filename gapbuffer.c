#include "everything.h"

#define ABS(x) ((x) > 0 ? (x) : -(x))

void initGB(gapBuffer *gb, int initSize) {
    if ((gb->buffer = malloc(sizeof(char) * initSize)) == NULL) {
        perror("gapbuffer initialization");
        exit(1);
    }
        
    gb->size = initSize;
    gb->presize = 0;
    gb->postsize = 0;
}

void freeGB(gapBuffer *gb) {
    free(gb->buffer);
    free(gb);   
}
/* Expand buffer if the gap is all filled */
void expandGB(gapBuffer *gb) {
    int newsize = gb->size * 2;
    char* newBuf = (char *) malloc(sizeof(char) * newsize);
    if (!memcpy(newBuf, gb->buffer, gb->presize * sizeof(char)) || 
        !memcpy(&newBuf[newsize - gb->postsize], &gb->buffer[gb->size - gb->postsize], gb->postsize * sizeof(char))) {
        perror("failed to expand gapBuffer");
        exit(1);
        }
    free(gb->buffer);
    gb->buffer = newBuf;
    gb->size = newsize;
}

/* Move the cursor forward one character */
void moveCursorForwardGB(gapBuffer *gb) {
    if (gb->postsize > 0)  {
        gb->buffer[gb->presize] = gb->buffer[gb->size - gb->postsize];
        gb->presize++;
        gb->postsize--;
    }
}

/* Move the cursor backward one character */
void moveCursorBackwardGB(gapBuffer *gb) {
    if (gb->presize > 0) {
        gb->postsize++;
        gb->buffer[gb->size - gb->postsize] = gb->buffer[gb->presize - 1];
        gb->presize--;
    }
}

void shiftRightGB(gapBuffer *gb) 
/*
- Shift the two buffers to the right
*/
{
    if (gb->presize != 0 && !memcpy(&(gb->buffer[gb->size - gb->postsize - gb->presize]), gb->buffer, gb->presize)) {
        perror("memcpy - shiftRightGB");
        exit(1);
    }
    gb->postsize += gb->presize;
    gb->presize = 0;
}

void shiftLeftGB(gapBuffer *gb)
/*
- Shift the two buffers to the left
*/
{
    if (gb->postsize != 0 && !memcpy(&(gb->buffer[gb->presize]), &(gb->buffer[gb->size - gb->postsize]), gb->postsize)) {
        perror("memcpy - shiftLeftGB");
        exit(1);
    }
    gb->presize += gb->postsize;
    gb->postsize = 0;
}

/* Move the cursor an arbitrary length */
void moveNCursorGB(gapBuffer *gb, int n) {
    if (n > 0) {
        while (n-- != 0)
            moveCursorForwardGB(gb);
    } else {
        while (n++ != 0)
            moveCursorBackwardGB(gb);
    }
}

/* Move cursor with index */
void moveCursorToIdxGB(gapBuffer *gb, int idx) 
/* 
- Use memmove() for overlapping buffer
*/
{
    int moveRange = idx - gb->presize; // how many characters to shift

    // if cursor moves forward
    if (moveRange > 0) {
        // too much forward
        moveRange = (moveRange <= gb->postsize) ? moveRange : gb->postsize;
        
        memmove(&gb->buffer[gb->presize], &gb->buffer[gb->size - gb->postsize], moveRange);
        gb->presize += moveRange;
        gb->postsize -= moveRange;
    } 
    // if cursor moves backward
    else if (moveRange < 0) {
        moveRange = -moveRange;
        moveRange = (moveRange <= gb->presize) ? moveRange : gb->presize;

        memmove(&gb->buffer[gb->size - gb->postsize - moveRange], &gb->buffer[gb->presize - moveRange], moveRange);
        gb->presize -= moveRange;
        gb->postsize += moveRange;
    }

}


/* Insert a character into buffer */
void insertCharGB(gapBuffer *gb, char c) {
    if ((gb->presize + gb->postsize) >= gb->size - 1)
        expandGB(gb);
    gb->buffer[gb->presize++] = c;
}

/* Delete a character from buffer */
void deleteCharGB(gapBuffer* gb) {
    if (gb->presize > 0)
        gb->presize--;
}


void insertStringGB(gapBuffer *gb, char *s, int len) 
/*
- Insert string to the right most side of buffer
*/
{   
    if (gb->size < len) {
        gb->size = (int) len * 1.5;
        gb->buffer = malloc((int) sizeof(char) * gb->size);
    }

    if (!memcpy(&gb->buffer[gb->size - len], s, len)) {
        perror("failed to initialize string for gapBuffer");
        exit(1);
    }

    gb->presize = 0;
    gb->postsize = len; // initialize the cursor to the start
}

void displayGB(gapBuffer *gb) {
    write(STDOUT_FILENO, gb->buffer, gb->presize);
    write(STDOUT_FILENO, &gb->buffer[gb->size - gb->postsize], gb->postsize);
}

int getLengthGB(gapBuffer *gb) {
    return gb->presize + gb->postsize;
}

int mapGB(char *dst, gapBuffer *gb, int n) {
    // check if dst buffer size is big enough
    if (n < getLengthGB(gb)) {  
        return 0;
    }
    if (gb->presize > 0 && !memcpy(dst, gb->buffer, gb->presize))
        return 0;

    if (gb->postsize > 0 && !memcpy(&dst[gb->presize], &gb->buffer[gb->size - gb->postsize], gb->postsize))
        return 0;  

    return 1;
}

int getCursorPosGB(gapBuffer *gb) {
    return gb->presize;
}

int getCursorNextPosGB(gapBuffer *gb) {
    return gb->presize;
}

void displayStringGB(gapBuffer *gb) {
    printf("Size - %d,max buffer size: %d, %.*s%.*s", gb->presize + gb->postsize, gb->size, gb->presize, gb->buffer, gb->postsize, &gb->buffer[gb->size - gb->postsize]);
}

int isCursorLeftGB(gapBuffer *gb) {
    if (gb->presize == 0)
        return 1;
    return 0;
}

int isCursorRightGB(gapBuffer *gb) {
    if (gb->postsize == 0)
        return 1;
    return 0;
}
/*  
- Combine two gapBuffers while keeping gap position in the middle *
- Two gapBuffers must be first shifted to both sides       
*/
gapBuffer* combineGB(gapBuffer *lGB, gapBuffer *rGB) {
    char* lBuf = NULL;
    char* rBuf = NULL;
    int lLength;
    int rLength;

    gapBuffer* newGB = (gapBuffer *) malloc(sizeof(gapBuffer));

    shiftLeftGB(lGB);
    shiftRightGB(rGB);

    lLength = lGB->presize;
    rLength = rGB->postsize;

    lBuf = lGB->buffer;
    rBuf = &rGB->buffer[rGB->size - rGB->postsize];

    initGB(newGB, lLength + rLength + 1);

    if (!memcpy(newGB->buffer, lBuf, lLength)) {
        perror("failed to combine gapBuffers");
        exit(1);
    }
    newGB->presize = lLength;

    if (!memcpy(&newGB->buffer[newGB->size - rLength], rBuf, rLength)) {
        perror("failed to combine gapBuffers");
        exit(1);
    }
    newGB->postsize = rLength;

    return newGB;
}

gapBuffer* splitGB(gapBuffer *GB) {
    gapBuffer* newGB = (gapBuffer *) malloc(sizeof(gapBuffer));
    char *temp = (char *) &GB->buffer[GB->size - GB->postsize];
    initGB(newGB, (int) GB->postsize * 1.5);

    insertStringGB(newGB, &GB->buffer[GB->size - GB->postsize], GB->postsize);
    GB->postsize = 0;

    return newGB;
}