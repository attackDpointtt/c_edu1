#include "hash_table.h"

// 哈希函数：将字符串 key 映射到 [0, TABLE_SIZE) 范围内的索引
static unsigned int hash(const char *key) {
    unsigned int h = 0;
    while (*key) {
        h = h * 31 + (unsigned char)(*key);
        key++;
    }
    return h % TABLE_SIZE;
}

// 创建一个新的哈希表
HashTable *create_hash_table() {
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    if (table == NULL) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    memset(table->buckets, 0, sizeof(table->buckets));
    return table;
}

// 释放哈希表及其所有节点
void destroy_hash_table(HashTable *table) {
    if (table == NULL) return;
    for (int i = 0; i < TABLE_SIZE; i++) {
        Node *cur = table->buckets[i];
        while (cur != NULL) {
            Node *next = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            cur = next;
        }
    }
    free(table);
}

// 插入或更新键值对
void put(HashTable *table, const char *key, const char *value) {
    if (table == NULL || key == NULL || value == NULL) return;

    unsigned int idx = hash(key);
    Node *cur = table->buckets[idx];

    // 检查 key 是否已存在，若存在则更新 value
    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0) {
            free(cur->value);
            cur->value = strdup(value);
            return;
        }
        cur = cur->next;
    }

    // key 不存在，创建新节点并插入链表头部
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    new_node->key = strdup(key);
    new_node->value = strdup(value);
    new_node->next = table->buckets[idx];
    table->buckets[idx] = new_node;
}

// 根据 key 查找 value，找不到返回 NULL
char *get(HashTable *table, const char *key) {
    if (table == NULL || key == NULL) return NULL;

    unsigned int idx = hash(key);
    Node *cur = table->buckets[idx];

    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0) {
            return cur->value;
        }
        cur = cur->next;
    }
    return NULL;
}

// 删除指定 key 的键值对
void remove_key(HashTable *table, const char *key) {
    if (table == NULL || key == NULL) return;

    unsigned int idx = hash(key);
    Node *cur = table->buckets[idx];
    Node *prev = NULL;

    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0) {
            if (prev == NULL) {
                // 删除的是链表头节点
                table->buckets[idx] = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur->key);
            free(cur->value);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}