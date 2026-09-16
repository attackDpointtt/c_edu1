#include "graph.h"

void floyd_warshall(int vertices) {
    int distance[MAX_VERTICES][MAX_VERTICES];

    // Initialize the distance matrix
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            if (i == j) {
                distance[i][j] = 0;
            } else if (adjMatrix[i][j] != 0) {
                distance[i][j] = adjMatrix[i][j];
            } else {
                distance[i][j] = INT_MAX;
            }
        }
    }

    // Floyd-Warshall algorithm
    for (int k = 0; k < vertices; k++) {
        for (int i = 0; i < vertices; i++) {
            for (int j = 0; j < vertices; j++) {
                if (distance[i][k] != INT_MAX && distance[k][j] != INT_MAX && distance[i][k] + distance[k][j] < distance[i][j]) {
                    distance[i][j] = distance[i][k] + distance[k][j];
                }
            }
        }
    }

    // Print the shortest distances
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            if (distance[i][j] == INT_MAX) {
                printf("INF ");
            } else {
                printf("%d ", distance[i][j]);
            }
        }
        printf("\n");
    }
}