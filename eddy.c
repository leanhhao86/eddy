#include "everything.h"

void editorMoveCursorTo(int);
void statusBarSet(char*);
int getCursorPosition(int *rows, int *cols);
void saveToFile(char*);
int editorReadKey();
void bufferMoveCursor(gapBuffer*, int);
unsigned char* editorUpdateSyntaxHighlight(char*, unsigned char*, int);
int syntaxToColor(int);
void die(const char*);

/*** version ***/

#define EDDY_VERSION "0.0.1"

/*** macros ***/
#define FILENAMELEN 50
#define CTRL_KEY(k) ((k) & 0x1f)

/* Special Chars */
#define NEWLINE '\n'
#define TAB     '\t'
#define TABSTOP 8

/* Highlighting flags */

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    DEL_KEY,
};

struct editorSyntax {
    char* filetype;
    char** filematch;
    char** keywords;
    int flags;
};

enum editorHighLight {
    HL_NORMAL = 0,
    HL_NUMBER,
    HL_MATCH,
    HL_STRING,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2
};

/*** filetypes ***/

char* C_HL_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {
    {
        "c",
        C_HL_extensions,
        C_HL_keywords,
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    }
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** Editor ***/

struct editorConfig {
    struct termios orig_termios;
    char* filename;
    int screenrows;
    int screencols;
    int rowToDisplay;       /* index of the highest displayable row */
    int cx, cy;
    GBNode *rootNode;  
    GBNode *lastNode;
    GBNode *curNode;
    int numlines;
    int curline;
    appendBuffer* ab;
    char* statusMessage;
    int dirty;
    int quitTime;
    rowInfo* rInfo;
    struct editorSyntax *syntax;  
};

struct editorConfig E;

/*** buffer processing ***/

int editorMapAB() 
/*
* Process the buffer line by line
* Tab processing
* 
*/
{
    int line, idx, len, clen; 
    int nSpace;                 // number of spaces to tabstop
    int colIdx, rowIdx;         // index into every row of screen
    int isCursorProcessed = 0;  // check if the cursor is processed
    int currentLogicCursorX = getCursorPosGB(E.curNode->GB);
    GBNode *tmp = E.rootNode;
    appendBuffer* ab = E.ab;
    char* tempBuffer = NULL;    // current buffer to be processed
    char colorStr[16];          // colorStr
    int currentColor = -1, color;
    /* reset the the mark array*/
    E.rInfo->numMarks = 0;
    ab->len = 0;
    
    rowIdx = 0;
    updateRowInfo(E.rInfo, 0);
    for (line = 0; line < E.numlines; line++, tmp=tmp->next) {
        tempBuffer = tmp->buffer;
        len = getLengthGB(tmp->GB);
        
        colIdx = 0;
        for (idx = 0; idx < len; idx++) {
            /* visual cursor processing */
            if (!isCursorProcessed && E.curline == line && currentLogicCursorX == idx) {
                E.cx = colIdx;
                E.cy = rowIdx;
                isCursorProcessed = 1;
            }   
            /* tab processing */
            if (tempBuffer[idx] == '\t') {
                nSpace = (idx > TABSTOP) ? (idx % TABSTOP) : (TABSTOP - idx);

                while (nSpace != 0) {
                    if (colIdx < E.screencols - 1) {
                        appendCharAB(ab, ' ');
                        colIdx++;
                        nSpace--;
                    } else {
                        appendCharAB(ab, '\n');
                        colIdx = 0;
                        rowIdx++;
                        nSpace = 0;
                        updateRowInfo(E.rInfo, ab->len);
                    }
                }
            } 
            else {
                if (tmp->hl[idx] == HL_NORMAL) {
                    // appendAB(ab, "\x1b[39m", 5);
                    if (currentColor != -1) {
                        appendAB(ab, "\x1b[39m", 5);
                        currentColor = -1;  
                    }
                    appendCharAB(ab, tempBuffer[idx]);
                } else {
                    color = syntaxToColor(tmp->hl[idx]);
                    if (color != currentColor) {
                        currentColor = color;
                        clen = snprintf(colorStr, 16, "\x1b[%dm", color);
                        appendAB(ab, colorStr, clen);
                    }
                    appendCharAB(ab, tempBuffer[idx]);
                }

                if (tempBuffer[idx] == NEWLINE) {
                    rowIdx++;
                    colIdx = 0;
                    updateRowInfo(E.rInfo, ab->len);
                } 
                /* normal character */
                else {
                    colIdx++;
                }
            }
            if (colIdx >= E.screencols) {
                rowIdx++;
                colIdx = colIdx % E.screencols;
                updateRowInfo(E.rInfo, ab->len);
            }

        }
    }
    appendAB(ab, "\x1b[39m", 5);
    return 1;
}

void moveCursorNodeGB(int nNode, int pos) {
    int i;
    GBNode* tempNode = NULL;

    if (nNode >= 0 && nNode < E.numlines) {
        tempNode = getNodeAtIndex(E.rootNode, nNode);
        // check if pos is valid
        if (pos < 0 || pos >= getLengthGB(tempNode->GB))
            die("failed to move cursor to specific line");
        moveCursorToIdxGB(tempNode->GB, pos);
        E.curNode = tempNode;
        E.curline = nNode;        

        for (i = 0, tempNode = E.rootNode; i < nNode; i++, tempNode = tempNode->next) {
            shiftLeftGB(tempNode->GB);
        }
        i++;
        tempNode = tempNode->next;
        for (; i < E.numlines; i++, tempNode = tempNode->next) {
            shiftRightGB(tempNode->GB);
        }
    }
}

void freeAll() {
    // free append buffer
    freeAB(E.ab);
    // free rowInfo
    free(E.rInfo);
    // free status message
    free(E.statusMessage);
    // free every node 
    GBNode *nextNode = NULL;
    GBNode *currentNode = E.rootNode;
    while (currentNode != NULL) {
        nextNode = currentNode->next;
        freeGBNode(currentNode);
        currentNode = nextNode;
    }
    // free filename
    if (E.filename) free(E.filename);
}

/*** terminal ***/

void die(const char* s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); 	// refresh the screen
	write(STDOUT_FILENO, "\x1b[2H", 3);		// move the cursor to the start

    freeAll();
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		die("tcsetattr");
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;

	raw.c_iflag &= ~(IXON) | IGNCR | ICRNL;
	// raw.c_oflag &= XTABS;	
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
	raw.c_cc[VMIN] = 0;    
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // a fallback solution if ioctl not working
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** syntax highlighting ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int syntaxToColor(int hl) {
    switch(hl) {
        case HL_NUMBER:     return 31;  // red
        case HL_MATCH:      return 34;  // blue
        case HL_STRING:     return 35;  // pink
        case HL_MLCOMMENT:  return 36;  // cyan
        case HL_KEYWORD1:   return 33;
        case HL_KEYWORD2:   return 32;
        default:            return 37;  // white
    }
}

void editorSelectSyntaxHighLight() {
    E.syntax = NULL;
    unsigned int i, j;
    int is_ext;
    struct editorSyntax* s = NULL;

    if (E.filename == NULL) return;

    char* ext = strrchr(E.filename, '.');

    for (j = 0; j < HLDB_ENTRIES; j++) {
        s = &HLDB[j];
        // traverse all match strings
        for (i = 0; s->filematch[i]; i++) {
            is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) || 
                (!is_ext && strstr(E.filename, s->filematch[i]))) {
                E.syntax = s;
                // update the file highlight
                GBNode* tempNode;
                for (tempNode = E.rootNode; tempNode != NULL; tempNode = tempNode->next) {
                    tempNode->hl = editorUpdateSyntaxHighlight(tempNode->buffer, tempNode->hl, getLengthGB(tempNode->GB));
                } 
            }
        } 
    }
}

unsigned char* editorUpdateSyntaxHighlight(char* buffer, unsigned char* hl, int len) {
    hl = realloc(hl, len * sizeof(unsigned char));
    memset(hl, HL_NORMAL, len);
    if (E.syntax == NULL) return hl;
    
    int i;
    int prev_sep = 1;
    int in_string = 0;
    char c;
    unsigned char prev_hl;
    char** keywords = E.syntax->keywords;


    i = 0;
    while (i < len) {
        c = buffer[i];
        prev_hl = (i > 0) ? hl[i - 1] : HL_NORMAL;

        if (E.syntax->flags & (HL_HIGHLIGHT_STRINGS)) {
            if (in_string) {
                hl[i] = HL_STRING;
                if (c == '\\' && i+1 < len) {
                    hl[i+1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } 
            else if (c == '"' || c == '\'') {
                in_string = c;
                hl[i] = HL_STRING;
                i++;
                continue;
            }
        }

        if (E.syntax->flags & (HL_HIGHLIGHT_NUMBERS)) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                (c == '.' && prev_hl == HL_NUMBER)) {
                hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }

        if (prev_sep) {
          int j;
          for (j = 0; keywords[j]; j++) {
            int klen = strlen(keywords[j]);
            int kw2 = keywords[j][klen - 1] == '|';
            if (kw2) klen--;
            if (!strncmp(&buffer[i], keywords[j], klen) &&
                is_separator(buffer[i + klen])) {
              memset(&hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen * sizeof(char));
              i += klen;
              break;
            }
          }
          if (keywords[j] != NULL) {
            prev_sep = 0;
            continue;
          }
        }

        prev_sep = is_separator(c);
        i++;
    }
    return hl;
}



/*** output ***/

void statusBarSet(char *msg) {
    if (msg) {
        memset(E.statusMessage, 0, sizeof(char) * E.screencols+1);
        snprintf(E.statusMessage, E.screencols, "\x1b[%d;%dH%s", E.screenrows+1, 1, msg);
    }
}

void editorDisplayCursor() {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH\x1b[?25h", E.cy + 1, E.cx + 1);
    write(STDOUT_FILENO, buf, strlen(buf));
}


void editorRefreshScreen(int jumped) {
    static int lastDirty = 0;
    appendBuffer *ab = E.ab;
    int rowRange, moveRange, start, end;
    int startIdx, endIdx;
    char msg[200];
    int x, y;
    int frameChanged = 0;

    write(STDOUT_FILENO, "\x1b[?25l", 6);      // hide the cursor to prevent glitching
    // write(STDOUT_FILENO, "\x1b[2J", 4);        // clear the entire screen
    write(STDOUT_FILENO, "\x1b[H", 3);         // move cursor to position (1, 1)

    if (!editorMapAB())
        die("cannot map AB");

    x = E.cx; 
    y = E.cy;
    /* if frame jumped */
    if (jumped) {
        E.rowToDisplay = E.cy - E.screenrows;
        if (E.rowToDisplay < 0) E.rowToDisplay = 0;
        frameChanged = 1;
        // write(STDOUT_FILENO, "\x1b[2J", 4);        // clear the entire screen
    }
    /* calculate the current end row */
    end = E.rowToDisplay + E.screenrows - 1;
    if (end >= E.rInfo->numMarks) {
        end = E.rInfo->numMarks - 1;
    }
    /* if frame [start end] does not fit, shift it */

    if (E.cy > end) {
        E.rowToDisplay += 1;
        E.cy -= E.rowToDisplay;
        frameChanged = 1;
        // write(STDOUT_FILENO, "\x1b[2J", 4);        // clear the entire screen
    }
    else if (E.cy < E.rowToDisplay) {
        E.rowToDisplay -= 1;
        E.cy = 0;
        frameChanged = 1;
        // write(STDOUT_FILENO, "\x1b[2J", 4);        // clear the entire screen
    } else {
        E.cy -= E.rowToDisplay;
    }

    if (lastDirty != E.dirty) {
        frameChanged = 1;
        lastDirty = E.dirty;
    }
    
    end = E.rowToDisplay + E.screenrows - 1;
    if (end < E.rInfo->numMarks - 1) {
        endIdx = E.rInfo->rowMarks[end + 1];
    } else {
        endIdx = ab->len;
    }
    startIdx = E.rInfo->rowMarks[E.rowToDisplay];

    if(frameChanged) write(STDOUT_FILENO, "\x1b[2J", 4);        // clear the entire screen

    write(STDOUT_FILENO, &ab->buffer[startIdx], endIdx - startIdx);

    if (E.quitTime == 0) {
        sprintf(msg, "line: %d\t| Ctrl-H for help", E.curline);
        statusBarSet(msg);
    }
    write(STDOUT_FILENO, E.statusMessage, strlen(E.statusMessage));
    editorDisplayCursor();              // Make cursor reappear
    E.cx = x;
    E.cy = y;
}


/*** Input Processing ***/

char* editorPrompt(char* msg, int (*callback)(char*, int)) {
    int rx = 0;
    int c;
    int done = 0;
    char* buf = malloc(sizeof(char) * (E.screencols + 1));
    gapBuffer* gb = malloc(sizeof(gapBuffer));
    fflush(stdin);
    E.cx = 0;
    E.cy = E.screenrows+1;
    
    statusBarSet(msg);
    write(STDOUT_FILENO, E.statusMessage, strlen(E.statusMessage));

    initGB(gb, E.screencols + 1);
    while(!done) {
        // Process the message
        c = editorReadKey();
        switch(c) {
            case NEWLINE:
                done = 1;
                break;
            case TAB:
                break;
            case ARROW_LEFT:
            case ARROW_RIGHT:
                bufferMoveCursor(gb, c);
                break;
            case BACKSPACE:
                deleteCharGB(gb); 
                break;
            case '\x1b':
                done = 1;
                break;
            default:
                if (getLengthGB(gb) < E.screencols) {
                    insertCharGB(gb, c);
                }
                break;
        }
        // Refresh the status bar
        E.cx = 0;
        E.cy = E.screenrows+1;
        editorDisplayCursor();  
        // clear status line
        write(STDOUT_FILENO, "\x1b[2K", 4);
        mapGB(buf, gb, E.screencols+1);
        buf[getLengthGB(gb)] = '\0';
        statusBarSet(buf);
        write(STDOUT_FILENO, E.statusMessage, strlen(E.statusMessage));

        // do searching
        if (callback && callback(buf, c))
            E.quitTime = 1;
    } 
    // mapGB(buf, gb, E.screencols+1);
    freeGB(gb);
    return buf;
}

int editorFind(char* query, int key) 
/* Search for 'query' from last match
- key = 0 (default) : search from current position downward 
- key = ARROW_DOWN  : skip the current one
- key = ARROW_UP    : search from current position upward 
*/
{
    static int lastRow = -1;
    static int lastPos = -1;
    static int direction = -1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    int queryLen = strlen(query);
    GBNode* tempNode = NULL;
    gapBuffer* tempGB = NULL;
    char* buf = NULL;
    char* match = NULL;
    int curLine, offset;

    if (key == NEWLINE && saved_hl) {
        tempNode = getNodeAtIndex(E.rootNode, saved_hl_line);
        memcpy(tempNode->hl, saved_hl, getLengthGB(tempNode->GB));
        free(saved_hl);
        saved_hl = NULL;
        return 1;
    }

    if (queryLen == 0)
        return 0;

    switch(key) {
        case ARROW_RIGHT:
        case ARROW_DOWN:
            direction = 1;
            break;
        case ARROW_LEFT:
        case ARROW_UP:
            direction = -1;
            break;
        default:
            lastRow = -1;
            key = 0;
            if (saved_hl) {
                tempNode = getNodeAtIndex(E.rootNode, saved_hl_line);
                memcpy(tempNode->hl, saved_hl, getLengthGB(tempNode->GB));
                free(saved_hl);
                saved_hl = NULL;
            }
            break;
    }
    
    tempNode = E.curNode;
    curLine = E.curline;
    if (lastRow == -1)  {
        tempNode = E.rootNode;
        direction = 1;
        lastPos = 0;
        curLine = 0;
    }

    offset = lastPos;
    // search forward
    if (direction == 1) {
        for (; curLine < E.numlines; tempNode = tempNode->next, curLine++, offset = 0) {
            if ((getLengthGB(tempNode->GB) - offset + (key != 0 ? -1 : 0))  < queryLen)
                continue;

            match = strstr((char*) (tempNode->buffer + offset + (key != 0 ? 1 : 0)), query);
            if (match) {
                lastRow = curLine;
                lastPos = match - tempNode->buffer;
                break;
            }
        }
    } else {
        for (; curLine >= 0; tempNode = tempNode->prev, curLine--, offset = getLengthGB(tempNode->GB)) {
            if (offset < queryLen) {
                if (curLine == 0)
                    break;
                continue;
            }
            buf = malloc(sizeof(char) + offset + 1);
            strncpy(buf, tempNode->buffer, offset);
            buf[offset] = '\0';
            match = rstrstr(buf, query);
            if (match) {
                lastRow = curLine;
                lastPos = match - buf;
                
                free(buf);
                break;
            }
            free(buf);
            if(curLine == 0)
                break;
        }
    }

    if (match != NULL) {
        int bufLen = getLengthGB(tempNode->GB);
        saved_hl_line = lastRow;
        saved_hl = malloc(bufLen * sizeof(char));
        memcpy(saved_hl, tempNode->hl, bufLen);
        memset(&tempNode->hl[lastPos], HL_MATCH, queryLen);

        moveCursorNodeGB(lastRow, lastPos);
        editorRefreshScreen(1);
        return 1;
    }
    return 0;
}


int editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '8': return BACKSPACE;
                    }
                }
            } else {    
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                }
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    printf("\r\n");

    while (i < sizeof(buf) -1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

/* Move cursor position of gap buffer */
void bufferMoveCursor(gapBuffer *gb, int dir) {
    switch (dir) {
        case ARROW_LEFT:
            moveCursorBackwardGB(gb);
            break;
        case ARROW_RIGHT:
            moveCursorForwardGB(gb);
            break;
    }
}
/*  Received direction "dir" and move the cursor
*/
void moveCursor(int dir) 
{
    gapBuffer *currentGB = E.curNode->GB;
    int currentIdx = getCursorPosGB(currentGB);
    int len;
    
    if (dir == ARROW_LEFT) {
        if (currentIdx > 0) {
            bufferMoveCursor(currentGB, dir);
        } else if (E.curline > 0) {
            E.curNode = E.curNode->prev;
            E.curline--;
        }
    } else if (dir == ARROW_RIGHT) {
        if (currentIdx < getLengthGB(currentGB) - 1) {
            bufferMoveCursor(currentGB, dir);
        } else if (E.curline < E.numlines - 1) {
            E.curNode = E.curNode->next;
            E.curline++;
        }
    } 
    /* move up to the line above, if line is shorter then move to the end */
    else if (dir == ARROW_UP && E.curline > 0) {
        shiftRightGB(currentGB);
        E.curline--;
        E.curNode = E.curNode->prev;
        currentGB = E.curNode->GB;
        len = getLengthGB(currentGB) - 1;
        currentIdx = (len >= currentIdx) ? currentIdx : len;
        moveCursorToIdxGB(currentGB, currentIdx);
    }
    /* move down the line below, if line is shorter then move to the end */
    else if (dir == ARROW_DOWN && E.curline < E.numlines - 1) {
        shiftLeftGB(currentGB);
        E.curline++;
        E.curNode = E.curNode->next;
        currentGB = E.curNode->GB;
        len = getLengthGB(currentGB) - 1;
        currentIdx = (len >= currentIdx) ? currentIdx : len;
        moveCursorToIdxGB(currentGB, currentIdx);
    }
} 

void deleteChar() {
    GBNode* tempGBNode = E.curNode;
    gapBuffer *tempGB = E.curNode->GB;
    gapBuffer *newGB = NULL;

    if (tempGB->presize > 0) {
        tempGB->presize--;
    } else if (E.curline > 0) {
        E.curNode = E.curNode->prev;
        E.curline--;
        /* delete the newline character */
        deleteCharGB(E.curNode->GB);
        newGB = combineGB(E.curNode->GB, tempGB);
        freeGB(E.curNode->GB);

        E.curNode->GB = newGB;
        E.curNode->next = tempGBNode->next;
        if (E.curNode->next != 0x0)
            E.curNode->next->prev = E.curNode;
        freeGBNode(tempGBNode);
        E.numlines--;
    }
    updateGBNodeBuffer(E.curNode);
    E.curNode->hl = editorUpdateSyntaxHighlight(E.curNode->buffer, E.curNode->hl, getLengthGB(E.curNode->GB));
}

void insertChar(char c) {
    gapBuffer* newGB = NULL;
    GBNode* newGBNode = NULL;
    if (c != NEWLINE) {
        insertCharGB(E.curNode->GB, c);
        updateGBNodeBuffer(E.curNode);
        E.curNode->hl = editorUpdateSyntaxHighlight(E.curNode->buffer, E.curNode->hl, getLengthGB(E.curNode->GB));
    } else {   
        insertCharGB(E.curNode->GB, c);
        newGB = splitGB(E.curNode->GB);
        // update the old node's buffer
        updateGBNodeBuffer(E.curNode);
        E.curNode->hl = editorUpdateSyntaxHighlight(E.curNode->buffer, E.curNode->hl, getLengthGB(E.curNode->GB));
        newGBNode = malloc(sizeof(GBNode));
        initGBNode(newGBNode, newGB);
        insertGBNodeAfter(E.curNode, newGBNode);
        E.curNode = E.curNode->next;
        E.curline++;
        E.numlines++;
        // update the new node's buffer
        updateGBNodeBuffer(E.curNode);
        E.curNode->hl = editorUpdateSyntaxHighlight(E.curNode->buffer, E.curNode->hl, getLengthGB(E.curNode->GB));
    }   
}

void editorProcessKeypress() {
	int c = editorReadKey();
    appendBuffer *ab = E.ab;
    char* filename = NULL;

    if (c == CTRL_KEY('q')) {
        if (E.dirty) {
            E.quitTime++;
            statusBarSet("File not saved, hit 'Ctrl-Q' 3 times to quit anyway");
        }
        if (!E.dirty || E.quitTime == 3) {
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);  
            freeAll();
            exit(0);
        }
        return;
    }
    E.quitTime = 0;
	switch(c) {
        case CTRL_KEY('s'):
            if (E.dirty) {
                filename = E.filename;
                if (filename == NULL)
                    filename = editorPrompt("Enter filename", NULL);
                if (filename && strlen(filename) > 0) {
                    E.filename = filename;
                    saveToFile(E.filename);
                    E.dirty = 0;
                } else {
                    free(filename);
                    E.dirty = 1;
                }
            }
            break;
        case CTRL_KEY('h'):
            statusBarSet("Ctrl-Q to quit | Ctrl-S to save | Ctrl-X to save as | Ctrl-F to search");
            E.quitTime = -1;
            break;
        case CTRL_KEY('x'):
            filename = editorPrompt("Enter filename", NULL);
            saveToFile(filename);
            free(filename);
            break;
        case CTRL_KEY('f'):
            editorPrompt("Enter query", editorFind);
            break;
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_UP:
        case ARROW_DOWN:
            moveCursor(c);
            break;
        case BACKSPACE:
            deleteChar();
            E.dirty++;
            break;   
        default:
            if (c == NEWLINE || c != '\x1b') {
                insertChar(c);
                E.dirty++;
            }
            break;
	}

}

/*** init ***/

void initEditor(int abInitialSize) {
    E.cx = 0;
    E.cy = 0;
    E.rowToDisplay = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
    /* preserve a row for status bar */
    E.screenrows -= 1; 
    E.ab = (appendBuffer *) malloc(sizeof(appendBuffer));
    initAB(E.ab, abInitialSize * 2);
    E.rInfo = malloc(sizeof(rowInfo));
    initRowInfo(E.rInfo, E.numlines);
    E.statusMessage = malloc(sizeof(char) * E.screencols + 10);
    E.dirty = 0;
    E.quitTime = 0;
    E.syntax = NULL;
}

int editorInitBuffer(char *buffer, int len) 
/*
- Creat linked list of gapbuffers
*/
{
    int idx, start, end;
    int linelen;    
    gapBuffer* gb;
    GBNode tempRoot;
    tempRoot.next = NULL;
    GBNode *temp = &tempRoot;
    GBNode *newNode;

    start = 0;
    end = 0;
    E.numlines = 0;
    for (idx = 0; idx < len; idx++) {
        //  make a line if there's a newline 
        if (buffer[idx] == '\n' || idx == len-1) {
            end = idx+1;
            linelen = end - start;

            if ((newNode = (GBNode *) malloc(sizeof(GBNode))) == NULL ||
            (gb = (gapBuffer *) malloc(sizeof(gapBuffer))) == NULL) {
                perror("failed to allocate space for newNode");
                exit(1);
            }

            initGB(gb, linelen+1);
            insertStringGB(gb, &buffer[start], linelen);
            initGBNode(newNode, gb);
            newNode->hl = editorUpdateSyntaxHighlight(newNode->buffer, newNode->hl, linelen);

            temp = insertGBNodeAfter(temp, newNode);        
            E.numlines++;

            start = end;
        }
    }

    // Attach root Node;
    E.rootNode = tempRoot.next;
    E.rootNode->prev = NULL;
    if (E.rootNode->next != NULL)
        E.rootNode->next->prev = E.rootNode;
    E.curNode = E.rootNode;
    E.curline = 0;

    return 1;
}

void editorOpen(char* filename) {
    FILE *fp = NULL;
    char *buffer = NULL;
    size_t newsize;
    E.filename = NULL;

    if (filename != NULL) {
        E.filename = malloc(sizeof(char) * FILENAMELEN);
        strncpy(E.filename, filename, FILENAMELEN);

        fp = fopen(E.filename, "r");
        if (fp) {
            if (fseek(fp, 0, SEEK_END) == 0) {
                long bufsize = ftell(fp);
                if (bufsize == -1) {
                    die("ftell");
                }
                if (!(buffer = malloc(sizeof(char) * (bufsize+1))))
                    die("malloc");
                if (fseek(fp, 0, SEEK_SET) != 0)
                    die("fseek");
                newsize = fread(buffer, sizeof(char), bufsize, fp);
                if (ferror(fp) != 0)
                    die("trying to read file");
                else
                {
                    buffer[newsize++] = '\0';
                }
            }
            fclose(fp);
        } else {
            die("cannot open file");
        }
        // Add newline at the end for easier processing
        if (buffer[newsize-2] != NEWLINE) {
            buffer[newsize-1] = NEWLINE;
            buffer[newsize++] = '\0';
        }
    } else {
        newsize = 2;
        buffer = malloc(newsize * sizeof(char));
        buffer[0] = '\n';
        buffer[1] = '\0';
    }
    enableRawMode();
    editorInitBuffer(buffer, newsize-1);
    initEditor(newsize);
    editorSelectSyntaxHighLight();
    free(buffer);
}



void saveToFile(char* filename) {
    FILE* fp = NULL;
    char *buffer = NULL;
    GBNode* tempNode = NULL;
    size_t newFileSize = 0;
    int i, len, curlen;

    if(filename == NULL)
        return;


    for (i = 0, tempNode = E.rootNode; i < E.numlines && tempNode != NULL; i++, tempNode = tempNode->next) {
        newFileSize += getLengthGB(tempNode->GB);
    }
    if (!(buffer = malloc(sizeof(char) * newFileSize)))
        die("cannot allocate space for writing");

    for (len = 0, i = 0, tempNode = E.rootNode; i < E.numlines && tempNode != NULL; i++, tempNode = tempNode->next) {
        curlen = getLengthGB(tempNode->GB);
        memcpy(&buffer[len], tempNode->buffer, curlen);
        // mapGB(&buffer[len], tempNode->, newFileSize);
        len += curlen;
    }

    fp = fopen(filename, "w+");
    if(fp) {
        if (fwrite(buffer, len, sizeof(char), fp)) {
            if (ferror(fp))
                die("failed to write to file");
        }
        fclose(fp);
    }
    E.dirty = 0;

    editorSelectSyntaxHighLight();
}

int main(int argc, char** argv) {
    char* filename = NULL;
    if (argc > 1) {
        filename = argv[1];
    }
    
    editorOpen(filename);
    editorRefreshScreen(1);
	while (1) {
		editorRefreshScreen(0);
		editorProcessKeypress();
	}
	return 0;
}
