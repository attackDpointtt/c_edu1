#include <stdio.h>
#include <stdlib.h>

typedef enum {
    RED,
    BLACK
} Color;

typedef struct Node {
    int data;
    Color color;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;

Node *createNode(int data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->color = RED;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->parent = NULL;
    return newNode;
}