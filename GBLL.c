#include "everything.h"

void initGBNode(GBNode* newNode, gapBuffer* gb) {
    newNode->GB = gb;
    newNode->buffer = NULL;
    newNode->hl = NULL;
    newNode->prev = NULL;
    newNode->next = NULL;
    updateGBNodeBuffer(newNode);
}

GBNode* insertGBNodeLast(GBNode* root, GBNode* theNode) {
	GBNode *temp = NULL;
	
	if (root == NULL)
		return theNode;
	temp = root;

	if (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = theNode;
	theNode->prev = temp;

	return root;
}

GBNode* insertGBNodeFirst(GBNode* root, GBNode* theNode) {
	GBNode *temp = NULL;

	if (root != NULL)
		theNode->next = root;

	return theNode;
}

GBNode* insertGBNodeAfter(GBNode* root, GBNode* theNode) {
	if (root != NULL) {
		theNode->prev = root;
		if (root->next == NULL) 
			root->next = theNode;
		else {
			theNode->next = root->next;
			root->next = theNode;
			theNode->next->prev = theNode;
		}
	}
	return theNode;
}

int insertGBNodeAtIndex(GBNode* root, GBNode* theNode, int index) {
	int i;
	GBNode *temp;

	temp = root;
	for (i = 0; i < index && temp->next != NULL; i++, temp = temp->next) 
		;

	if (i < index) 
		return 0;
	theNode->prev = temp->prev;
	theNode->next = temp;
	temp->prev = theNode;

	return 1;
}

GBNode* getNodeAtIndex(GBNode* root, int idx) {
	int i;
	for (i = 0; i != idx && root != NULL; root = root->next, i++)
		;
	if (i == idx)
		return root;
	return NULL;
}

void updateGBNodeBuffer(GBNode* node) {
	int len = getLengthGB(node->GB) + 1;
	if (len > 1 && node->buffer != NULL)
		free(node->buffer);


	node->buffer = malloc(sizeof(char) * len);
	mapGB(node->buffer, node->GB, len);
}

void freeGBNode(GBNode *node) {
	free(node->GB);
	free(node->buffer);
	free(node->hl);
	free(node);
}
