#include "everything.h"

char* reverse(char* buffer, int len) {
	int i, j;
	char temp;
	char* newbuf = malloc(sizeof(char) * len);
	strncpy(newbuf, buffer, len);

	for (i = 0; i < len; i++) {
		newbuf[len - i - 1] = buffer[i];
	}
	return newbuf;
}

char* rstrstr(char* haystack, char* needle) {
	int haylen = strlen(haystack);
	int needlen = strlen(needle);
	char* match = NULL;
	int idx;

	if (haylen == 0 || needlen > haylen)
		return NULL;
	// printf("haylen - %d ; needlen - %d\n", haylen, needlen);

	char* rhaystack = reverse(haystack, haylen);
	char* rneedle = reverse(needle, needlen);
	// puts(rhaystack);
	// puts(rneedle);

	match = strstr(rhaystack, rneedle);
	if (match != NULL) {
		// printf("match idx - %d\n", match - rhaystack);
		idx = haylen - (match + needlen - rhaystack);
	}
	free(rhaystack);
	free(rneedle);
	return (match != NULL ? haystack + idx : NULL );
}

void initRowInfo(rowInfo* rInfo, int maxMarks) {
    rInfo->maxMarks = maxMarks;
    rInfo->numMarks = 0;
    rInfo->rowMarks = malloc(sizeof(int) * maxMarks);
}

void updateRowInfo(rowInfo* rInfo, int mark) {
    if (rInfo->numMarks >= rInfo->maxMarks) {
        rInfo->maxMarks *= 2;
        if ((rInfo->rowMarks = realloc(rInfo->rowMarks, sizeof(int) * rInfo->maxMarks)) == NULL) {
            perror("failed to expand rowMarks");
            exit(1);
        }
    }
   	rInfo->rowMarks[rInfo->numMarks++] = mark;
}



