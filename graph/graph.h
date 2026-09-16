#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_VERTICES 100
#define INFINITY INT_MAX

typedef struct Node{
    int vertex;
    int weight;
    struct Node* next;
} Node;

typedef struct Edge{
    int src;
    int dest;
    int weight;
} Edge;


extern int adjMatrix[MAX_VERTICES][MAX_VERTICES];
extern Node* adjList[MAX_VERTICES];
extern bool visited[MAX_VERTICES];


extern void floyd_warshall(int vertices);
extern void bellman_ford(int source, int vertices);
extern void prim(int vertices);
extern void kruskal(int vertices);

extern int adjMatrix[MAX_VERTICES][MAX_VERTICES];
extern Node* adjList[MAX_VERTICES];
extern bool visited[MAX_VERTICES];

extern int key[MAX_VERTICES];
extern int parent[MAX_VERTICES];
extern bool mstSet[MAX_VERTICES];

void initializeMatrix(int vertices);
void initializeList(int vertices);
void dfs(int vertex, int vertices);
void bfs(int vertex, int vertices);
void dijkstra(int source, int vertices);
void topoSort(int vertices);
void bellman_ford(int source, int vertices);
void kruskal(int vertices);


#endif // GRAPH_H