#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 10

typedef struct Node {
    int key;
    int value;
    struct Node* next;
} Node;

Node* hash_table[TABLE_SIZE];

int hash_function(int key) {
    return abs(key) % TABLE_SIZE;
}

void insert(int key, int value) {
    int index = hash_function(key);
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->key = key;
    new_node->value = value;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
}

int search(int key) {
    int index = hash_function(key);
    Node* current = hash_table[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next;
    }
    return -1; // Key not found
}

int main() {
    // Initialize the hash table
    for (int i = 0; i < TABLE_SIZE; i++) {
        hash_table[i] = NULL;
    }

    // Insert some key-value pairs
    insert(10, 100);
    insert(25, 250);
    insert(30, 300);
    insert(45, 450);
    insert(50, 500);
    insert(60, 600);
    insert(70, 700);

    // Search for elements
    int target = 45;
    int result = search(target);
    if (result != -1) {
        printf("Element found with value %d\n", result);
    } else {
        printf("Element not found\n");
    }

    return 0;
}