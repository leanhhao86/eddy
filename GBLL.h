/* doubly linked list data structure used for gap buffer */
#include "everything.h"

typedef struct GBNode {
	gapBuffer *GB;
	char* buffer;
	unsigned char* hl;
	struct GBNode *prev;
	struct GBNode *next;
} GBNode;

void initGBNode(GBNode*, gapBuffer*);
GBNode* insertGBNodeLast(GBNode*, GBNode*);
GBNode* insertGBNodeFirst(GBNode*, GBNode*);
GBNode* insertGBNodeAfter(GBNode*, GBNode*);
int insertGBNodeAtIndex(GBNode*, GBNode*, int);
GBNode* getNodeAtIndex(GBNode*, int);
void updateGBNodeBuffer(GBNode*);
void freeGBNode(GBNode*);
