#include "graph.h"

void dijkstra(int source, int vertices) {
    int distance[MAX_VERTICES];
    bool visited[MAX_VERTICES];

    for (int i = 0; i < vertices; i++) {
        distance[i] = INT_MAX;
        visited[i] = false;
    }

    distance[source] = 0;

    for (int i = 0; i < vertices; i++) {
        int minDistance = INT_MAX;
        int minVertex = -1;

        for (int j = 0; j < vertices; j++) {
            if (!visited[j] && distance[j] < minDistance) {
                minDistance = distance[j];
                minVertex = j;
            }
        }

        if (minVertex == -1) {
            break;
        }

        visited[minVertex] = true;

        Node* current = adjList[minVertex];
        while (current != NULL) {
            int neighbor = current->vertex;
            int weight = current->weight;

            if (!visited[neighbor] && distance[minVertex] + weight < distance[neighbor]) {
                distance[neighbor] = distance[minVertex] + weight;
            }

            current = current->next;
        }
    }

    for (int i = 0; i < vertices; i++) {
        printf("Distance from %d to %d is %d\n", source, i, distance[i]);
    }
}