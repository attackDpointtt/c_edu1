#include "graph.h"

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