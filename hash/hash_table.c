#include "hash_table.h"

HashTable *create_hash_table() {
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    for (int i = 0; i < TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    return table;
}

void destroy_hash_table(HashTable *table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Node *current = table->buckets[i];
        while (current != NULL) {
            Node *temp = current;
            current = current->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }
    }
    free(table);
}

unsigned int hash(const char *key) {
    unsigned int hash_value = 0;
    while (*key) {
        hash_value = hash_value * 31 + *key;
        key++;
    }
    return hash_value % TABLE_SIZE;
}

void put(HashTable *table, const char *key, const char *value) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            free(current->value);
            current->value = strdup(value);
            return;
        }
        current = current->next;
    }
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->key = strdup(key);
    new_node->value = strdup(value);
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
}

char *get(HashTable *table, const char *key) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

void remove_key(HashTable *table, const char *key) {
    unsigned int index = hash(key);
    Node *current = table->buckets[index];
    Node *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                table->buckets[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current->key);
            free(current->value);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}