#include "graph.h"

void bellman_ford(int source, int vertices) {
    int distance[MAX_VERTICES];

    for (int i = 0; i < vertices; i++) {
        distance[i] = INT_MAX;
    }
    distance[source] = 0;

    for (int i = 0; i < vertices - 1; i++) {
        for (int j = 0; j < vertices; j++) {
            if (distance[j] != INT_MAX) {
                Node* current = adjList[j];
                while (current != NULL) {
                    int neighbor = current->vertex;
                    int weight = current->weight;

                    if (distance[j] + weight < distance[neighbor]) {
                        distance[neighbor] = distance[j] + weight;
                    }

                    current = current->next;
                }
            }
        }
    }

    // Check for negative weight cycles
    for (int j = 0; j < vertices; j++) {
        if (distance[j] != INT_MAX) {
            Node* current = adjList[j];
            while (current != NULL) {
                int neighbor = current->vertex;
                int weight = current->weight;

                if (distance[j] + weight < distance[neighbor]) {
                    printf("Graph contains a negative weight cycle\n");
                    return;
                }

                current = current->next;
            }
        }
    }

    for (int i = 0; i < vertices; i++) {
        printf("Distance from %d to %d is %d\n", source, i, distance[i]);
    }
}