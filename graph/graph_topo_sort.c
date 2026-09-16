#include "graph.h"

void topoSort(int vertices) {
    int inDegree[MAX_VERTICES];
    for (int i = 0; i < vertices; i++) {
        inDegree[i] = 0;
    }

    for (int i = 0; i < vertices; i++) {
        Node* current = adjList[i];
        while (current != NULL) {
            inDegree[current->vertex]++;
            current = current->next;
        }
    }

    // Implementation for topological sorting
}