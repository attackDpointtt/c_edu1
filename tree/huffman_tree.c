#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    char data;
    int frequency;
    struct Node *left;
    struct Node *right;
} Node;

Node *createNode(char data, int frequency) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->frequency = frequency;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}