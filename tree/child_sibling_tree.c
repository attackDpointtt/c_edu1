#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* firstChild;
    struct Node* nextSibling;
} Node;

Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->firstChild = NULL;
    newNode->nextSibling = NULL;
    return newNode;
}