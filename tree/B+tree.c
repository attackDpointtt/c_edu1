#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define M 4  // B树的阶数（最大子节点数）
#define MAX_KEYS (M - 1)  // 每个节点最多关键字数
#define MIN_KEYS ((M + 1) / 2 - 1)  // 非根节点最少关键字数

// B树节点结构体
typedef struct BTreeNode {
    int keys[MAX_KEYS];              // 关键字数组
    struct BTreeNode *children[M];   // 子节点指针数组
    int num_keys;                    // 当前关键字数量
    bool is_leaf;                    // 是否为叶子节点
} BTreeNode;

// 创建新节点
BTreeNode *createNode(bool is_leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (!node) {
        perror("内存分配失败");
        exit(EXIT_FAILURE);
    }
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    for (int i = 0; i < M; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// 查找关键字，返回所在节点和位置
// 成功返回true，失败返回false
bool search(BTreeNode *node, int key, int *pos) {
    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) {
        i++;
    }
    if (i < node->num_keys && key == node->keys[i]) {
        *pos = i;
        return true;
    }
    if (node->is_leaf) {
        return false;
    }
    return search(node->children[i], key, pos);
}

// 分裂满子节点
// parent: 父节点, index: 满子节点在父节点中的索引, child: 满子节点
void splitChild(BTreeNode *parent, int index, BTreeNode *child) {
    BTreeNode *newNode = createNode(child->is_leaf);
    int mid = MAX_KEYS / 2;  // 中间关键字的索引（对于4阶树，mid=1）

    // 将child后半部分的关键字复制到newNode
    newNode->num_keys = mid;
    for (int i = 0; i < mid; i++) {
        newNode->keys[i] = child->keys[i + mid + 1];
    }

    // 如果child不是叶子节点，复制子节点指针
    if (!child->is_leaf) {
        for (int i = 0; i <= mid; i++) {
            newNode->children[i] = child->children[i + mid + 1];
        }
    }

    child->num_keys = mid;

    // 在父节点中为新节点腾出空间
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = newNode;

    // 在父节点中为中间关键字腾出空间
    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
    }

    // 将中间关键字上移到父节点
    parent->keys[index] = child->keys[mid];
    parent->num_keys++;
}

// 向非满节点插入关键字
void insertNonFull(BTreeNode *node, int key) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        // 叶子节点：找到插入位置并后移元素
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->num_keys++;
    } else {
        // 内部节点：找到应该插入的子树
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;

        // 如果子节点已满，先分裂
        if (node->children[i]->num_keys == MAX_KEYS) {
            splitChild(node, i, node->children[i]);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], key);
    }
}

// 插入关键字（入口函数）
BTreeNode *insert(BTreeNode *root, int key) {
    if (root == NULL) {
        return createNode(true);
    }

    // 如果根节点已满，需要分裂并创建新根
    if (root->num_keys == MAX_KEYS) {
        BTreeNode *newRoot = createNode(false);
        newRoot->children[0] = root;
        splitChild(newRoot, 0, root);
        insertNonFull(newRoot, key);
        return newRoot;
    }

    insertNonFull(root, key);
    return root;
}

// 从节点中移除关键字（叶子节点直接移除）
void removeFromLeaf(BTreeNode *node, int index) {
    for (int i = index; i < node->num_keys - 1; i++) {
        node->keys[i] = node->keys[i + 1];
    }
    node->num_keys--;
}

// 从节点中移除关键字（非叶子节点，用前驱或后继替代）
void removeFromNonLeaf(BTreeNode *node, int index) {
    int key = node->keys[index];

    // 如果左子树关键字足够多，用前驱替代
    if (node->children[index]->num_keys > MIN_KEYS) {
        // 找到左子树中的最大关键字（前驱）
        BTreeNode *pred = node->children[index];
        while (!pred->is_leaf) {
            pred = pred->children[pred->num_keys];
        }
        int predKey = pred->keys[pred->num_keys - 1];
        node->keys[index] = predKey;
        // 递归删除前驱
        // 简化处理：这里假设前驱一定在叶子中
        removeFromLeaf(pred, pred->num_keys - 1);
    }
    // 否则用后继替代
    else if (node->children[index + 1]->num_keys > MIN_KEYS) {
        BTreeNode *succ = node->children[index + 1];
        while (!succ->is_leaf) {
            succ = succ->children[0];
        }
        int succKey = succ->keys[0];
        node->keys[index] = succKey;
        removeFromLeaf(succ, 0);
    }
    // 左右子树都不够，合并
    else {
        merge(node, index);
        // 递归在合并后的子节点中删除
        deleteKey(node->children[index], key);
    }
}

// 合并两个子节点
void merge(BTreeNode *node, int index) {
    BTreeNode *left = node->children[index];
    BTreeNode *right = node->children[index + 1];
    int midKey = node->keys[index];

    // 将父节点的关键字下移到左子节点
    left->keys[left->num_keys] = midKey;
    left->num_keys++;

    // 将右子节点的关键字复制到左子节点
    for (int i = 0; i < right->num_keys; i++) {
        left->keys[left->num_keys] = right->keys[i];
        left->num_keys++;
    }

    // 复制子节点指针
    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++) {
            left->children[left->num_keys] = right->children[i];
            left->num_keys++;
        }
    }

    // 从父节点中移除关键字和右子节点指针
    for (int i = index; i < node->num_keys - 1; i++) {
        node->keys[i] = node->keys[i + 1];
        node->children[i + 1] = node->children[i + 2];
    }
    node->num_keys--;
    free(right);
}

// 借位：从右兄弟借一个关键字
void borrowFromRight(BTreeNode *node, int index) {
    BTreeNode *child = node->children[index];
    BTreeNode *rightSibling = node->children[index + 1];

    // 父节点的关键字下移到子节点末尾
    child->keys[child->num_keys] = node->keys[index];
    child->num_keys++;

    // 右兄弟的第一个关键字上移到父节点
    node->keys[index] = rightSibling->keys[0];

    // 右兄弟关键字左移
    for (int i = 0; i < rightSibling->num_keys - 1; i++) {
        rightSibling->keys[i] = rightSibling->keys[i + 1];
    }
    rightSibling->num_keys--;

    // 复制子节点指针
    if (!child->is_leaf) {
        child->children[child->num_keys] = rightSibling->children[0];
        for (int i = 0; i < rightSibling->num_keys; i++) {
            rightSibling->children[i] = rightSibling->children[i + 1];
        }
        rightSibling->children[rightSibling->num_keys] = NULL;
    }
}

// 借位：从左兄弟借一个关键字
void borrowFromLeft(BTreeNode *node, int index) {
    BTreeNode *child = node->children[index];
    BTreeNode *leftSibling = node->children[index - 1];

    // 关键字右移腾出位置
    for (int i = child->num_keys; i > 0; i--) {
        child->keys[i] = child->keys[i - 1];
    }
    if (!child->is_leaf) {
        for (int i = child->num_keys + 1; i > 0; i--) {
            child->children[i] = child->children[i - 1];
        }
    }

    // 父节点的关键字下移到子节点开头
    child->keys[0] = node->keys[index - 1];
    child->num_keys++;

    // 左兄弟的最后一个关键字上移到父节点
    node->keys[index - 1] = leftSibling->keys[leftSibling->num_keys - 1];
    leftSibling->num_keys--;

    // 复制子节点指针
    if (!child->is_leaf) {
        child->children[0] = leftSibling->children[leftSibling->num_keys + 1];
        leftSibling->children[leftSibling->num_keys + 1] = NULL;
    }
}

// 删除关键字
BTreeNode *deleteKey(BTreeNode *node, int key) {
    if (!node) return NULL;

    int index = 0;
    while (index < node->num_keys && key > node->keys[index]) {
        index++;
    }

    // 找到关键字
    if (index < node->num_keys && node->keys[index] == key) {
        if (node->is_leaf) {
            // 情况1：叶子节点，直接删除
            removeFromLeaf(node, index);
        } else {
            // 情况2：非叶子节点
            removeFromNonLeaf(node, index);
        }
    } else {
        // 关键字不在当前节点，在子节点中查找
        if (node->is_leaf) {
            printf("关键字 %d 不在树中\n", key);
            return node;
        }

        BTreeNode *child = node->children[index];

        // 如果子节点关键字不足，需要借位或合并
        if (child->num_keys == MIN_KEYS) {
            if (index > 0 && node->children[index - 1]->num_keys > MIN_KEYS) {
                borrowFromLeft(node, index);
            } else if (index < node->num_keys && node->children[index + 1]->num_keys > MIN_KEYS) {
                borrowFromRight(node, index);
            } else {
                if (index < node->num_keys) {
                    merge(node, index);
                } else {
                    merge(node, index - 1);
                    child = node->children[index - 1];
                }
            }
        }

        // 如果根节点没有关键字了，更新根
        if (node->num_keys == 0 && !node->is_leaf) {
            BTreeNode *newRoot = node->children[0];
            free(node);
            node = newRoot;
        }

        deleteKey(child, key);
    }

    return node;
}

// 中序遍历（输出有序序列）
void inorderTraversal(BTreeNode *node) {
    if (!node) return;
    for (int i = 0; i < node->num_keys; i++) {
        inorderTraversal(node->children[i]);
        printf("%d ", node->keys[i]);
    }
    inorderTraversal(node->children[node->num_keys]);
}

// 打印树结构（按层打印）
void printTree(BTreeNode *node, int level) {
    if (!node) return;
    printf("第 %d 层 [%d 个关键字]: ", level, node->num_keys);
    for (int i = 0; i < node->num_keys; i++) {
        printf("%d ", node->keys[i]);
    }
    printf("\n");
    for (int i = 0; i <= node->num_keys; i++) {
        printTree(node->children[i], level + 1);
    }
}

// 销毁树
void destroyTree(BTreeNode *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroyTree(node->children[i]);
        }
    }
    free(node);
}

int main() {
    BTreeNode *root = NULL;

    // 测试插入
    printf("=== 插入测试 ===\n");
    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17};
    int n = sizeof(keys) / sizeof(keys[0]);
    for (int i = 0; i < n; i++) {
        root = insert(root, keys[i]);
        printf("插入 %d 后:\n", keys[i]);
        printTree(root, 1);
        printf("中序遍历: ");
        inorderTraversal(root);
        printf("\n\n");
    }

    // 测试查找
    printf("=== 查找测试 ===\n");
    int pos;
    int searchKeys[] = {6, 15, 30};
    for (int i = 0; i < 3; i++) {
        if (search(root, searchKeys[i], &pos)) {
            printf("找到关键字 %d，位置: %d\n", searchKeys[i], pos);
        } else {
            printf("未找到关键字 %d\n", searchKeys[i]);
        }
    }

    // 测试删除
    printf("\n=== 删除测试 ===\n");
    int deleteKeys[] = {6, 12, 5};
    for (int i = 0; i < 3; i++) {
        printf("删除 %d:\n", deleteKeys[i]);
        root = deleteKey(root, deleteKeys[i]);
        printTree(root, 1);
        printf("中序遍历: ");
        inorderTraversal(root);
        printf("\n\n");
    }

    destroyTree(root);
    return 0;
}
