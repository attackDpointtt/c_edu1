#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 100

typedef struct Node {
    char *key;
    char *value;
    struct Node *next;
} Node;

typedef struct {
    Node *buckets[TABLE_SIZE];
} HashTable;

HashTable *create_hash_table();
void destroy_hash_table(HashTable *table);
void put(HashTable *table, const char *key, const char *value);
char *get(HashTable *table, const char *key);
void remove_key(HashTable *table, const char *key);

#endif // HASH_TABLE_H