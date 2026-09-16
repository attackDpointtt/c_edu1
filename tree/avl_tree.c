
#include <stdio.h>
#include <stdlib.h>

// 定义 AVL 树节点结构体
typedef struct Node {
    int data;
    int height;
    struct Node *left;
    struct Node *right;
} Node;

// 辅助函数：获取节点高度（空节点高度为 0）
int getHeight(Node *node) {
    if (node == NULL) return 0;
    return node->height;
}

// 辅助函数：获取节点平衡因子
int getBalanceFactor(Node *node) {
    if (node == NULL) return 0;
    return getHeight(node->left) - getHeight(node->right);
}

// 辅助函数：更新节点高度
void updateHeight(Node *node) {
    int lh = getHeight(node->left);
    int rh = getHeight(node->right);
    node->height = (lh > rh ? lh : rh) + 1;
}

// 核心操作：右旋 (Right Rotation)
// 适用于左左(LL)情况
Node *rightRotate(Node *y) {
    Node *x = y->left;
    Node *T2 = x->right;

    // 执行旋转
    x->right = y;
    y->left = T2;

    // 更新高度（注意：必须先更新 y，因为 y 现在变成了 x 的子节点）
    updateHeight(y);
    updateHeight(x);

    return x;
}

// 核心操作：左旋 (Left Rotation)
// 适用于右右(RR)情况
Node *leftRotate(Node *x) {
    Node *y = x->right;
    Node *T2 = y->left;

    // 执行旋转
    y->left = x;
    x->right = T2;

    // 更新高度
    updateHeight(x);
    updateHeight(y);

    return y;
}

// 插入节点
Node *insert(Node *node, int data) {
    // 1. 执行标准的 BST 插入
    if (node == NULL) {
        Node *newNode = (Node *)malloc(sizeof(Node));
        newNode->data = data;
        newNode->height = 1; // 新节点高度为 1
        newNode->left = NULL;
        newNode->right = NULL;
        return newNode;
    }

    if (data < node->data)
        node->left = insert(node->left, data);
    else if (data > node->data)
        node->right = insert(node->right, data);
    else // 不允许重复值
        return node;

    // 2. 更新当前节点的高度
    updateHeight(node);

    // 3. 获取平衡因子，检查是否失衡
    int balance = getBalanceFactor(node);

    // 4. 如果失衡，则进行相应的旋转处理
    // 左左 (LL) 情况
    if (balance > 1 && data < node->left->data)
        return rightRotate(node);

    // 右右 (RR) 情况
    if (balance < -1 && data > node->right->data)
        return leftRotate(node);

    // 左右 (LR) 情况
    if (balance > 1 && data > node->left->data) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // 右左 (RL) 情况
    if (balance < -1 && data < node->right->data) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

// 辅助函数：找到最小值节点（用于删除操作）
Node *minValueNode(Node *node) {
    Node *current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

// 删除节点
Node *deleteNode(Node *root, int data) {
    // 1. 标准的 BST 删除
    if (root == NULL) return root;

    if (data < root->data)
        root->left = deleteNode(root->left, data);
    else if (data > root->data)
        root->right = deleteNode(root->right, data);
    else {
        // 找到要删除的节点
        if ((root->left == NULL) || (root->right == NULL)) {
            Node *temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                // 没有子节点
                temp = root;
                root = NULL;
            } else {
                // 只有一个子节点
                *root = *temp; 
            }
            free(temp);
        } else {
            // 有两个子节点：获取中序后继（右子树的最小值）
            Node *temp = minValueNode(root->right);
            root->data = temp->data;
            root->right = deleteNode(root->right, temp->data);
        }
    }

    if (root == NULL) return root;

    // 2. 更新高度
    updateHeight(root);

    // 3. 检查平衡并旋转
    int balance = getBalanceFactor(root);

    // LL
    if (balance > 1 && getBalanceFactor(root->left) >= 0)
        return rightRotate(root);

    // LR
    if (balance > 1 && getBalanceFactor(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    // RR
    if (balance < -1 && getBalanceFactor(root->right) <= 0)
        return leftRotate(root);

    // RL
    if (balance < -1 && getBalanceFactor(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

// 中序遍历（用于验证排序）
void inorderTraversal(Node *root) {
    if (root != NULL) {
        inorderTraversal(root->left);
        printf("%d ", root->data);
        inorderTraversal(root->right);
    }
}

// 前序遍历（用于观察树的结构）
void preorderTraversal(Node *root) {
    if (root != NULL) {
        printf("%d ", root->data);
        preorderTraversal(root->left);
        preorderTraversal(root->right);
    }
}

int main() {
    Node *root = NULL;

    // 测试插入
    printf("插入序列: 10, 20, 30, 40, 50, 25\n");
    root = insert(root, 10);
    root = insert(root, 20);
    root = insert(root, 30); // 触发左旋
    root = insert(root, 40);
    root = insert(root, 50);
    root = insert(root, 25);

    printf("前序遍历: ");
    preorderTraversal(root);
    printf("\n中序遍历: ");
    inorderTraversal(root);
    printf("\n");

    // 测试删除
    printf("\n删除节点 30\n");
    root = deleteNode(root, 30);
    
    printf("前序遍历: ");
    preorderTraversal(root);
    printf("\n中序遍历: ");
    inorderTraversal(root);
    printf("\n");

    return 0;
}