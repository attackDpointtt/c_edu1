#include "graph.h"

int adjMatrix[MAX_VERTICES][MAX_VERTICES];
Node* adjList[MAX_VERTICES];
bool visited[MAX_VERTICES];

void initializeMatrix(int vertices) {
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            adjMatrix[i][j] = 0;
        }
    }
}

void initializeList(int vertices) {
    for (int i = 0; i < vertices; i++) {
        adjList[i] = NULL;
    }
}

void dfs(int vertex, int vertices) {
    visited[vertex] = true;
    printf("%d ", vertex);

    Node* current = adjList[vertex];
    while (current != NULL) {
        if (!visited[current->vertex]) {
            dfs(current->vertex, vertices);
        }
        current = current->next;
    }
}