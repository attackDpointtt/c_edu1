#include <stdio.h>
#include <stdlib.h>

typedef struct SegmentTree {
    int sum;
    int start;
    int end;
    struct SegmentTree *left;
    struct SegmentTree *right;
} SegmentTree;

SegmentTree *createSegmentTree(int start, int end) {
    SegmentTree *newNode = (SegmentTree *)malloc(sizeof(SegmentTree));
    newNode->start = start;
    newNode->end = end;
    newNode->sum = 0;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}