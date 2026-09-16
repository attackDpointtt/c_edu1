#include "graph.h"

void bfs(int vertex, int vertices) {
    bool visited[MAX_VERTICES] = {false};
    int queue[MAX_VERTICES];
    int front = 0, rear = 0;

    visited[vertex] = true;
    queue[rear++] = vertex;

    while (front < rear) {
        int current = queue[front++];
        printf("%d ", current);

        Node* neighbor = adjList[current];
        while (neighbor != NULL) {
            if (!visited[neighbor->vertex]) {
                visited[neighbor->vertex] = true;
                queue[rear++] = neighbor->vertex;
            }
            neighbor = neighbor->next;
        }
    }
}