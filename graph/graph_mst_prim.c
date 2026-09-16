#include "graph.h"

void prim(int vertices) {
    int key[MAX_VERTICES];
    bool mstSet[MAX_VERTICES];

    for (int i = 0; i < vertices; i++) {
        key[i] = INT_MAX;
        mstSet[i] = false;
    }

    key[0] = 0;

    for (int count = 0; count < vertices - 1; count++) {
        int u = -1;
        for (int i = 0; i < vertices; i++) {
            if (!mstSet[i] && (u == -1 || key[i] < key[u])) {
                u = i;
            }
        }

        mstSet[u] = true;

        Node* current = adjList[u];
        while (current != NULL) {
            int v = current->vertex;
            int weight = current->weight;

            if (!mstSet[v] && weight < key[v]) {
                key[v] = weight;
            }

            current = current->next;
        }
    }

    for (int i = 1; i < vertices; i++) {
        printf("Edge: %d - %d, Weight: %d\n", i, parent[i], key[i]);
    }
}