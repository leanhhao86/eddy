typedef struct rowInfo {
    int numMarks;
    int maxMarks;
    int* rowMarks;          /* indexes of where lines begin in buffer */
} rowInfo;

char* reverse(char*, int);
char* rstrstr(char*, char*);
void initRowInfo(rowInfo* rInfo, int);
void updateRowInfo(rowInfo*, int);
