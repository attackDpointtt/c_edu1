
/*
 * ============================================================================
 *  AVL 树（自平衡二叉搜索树）完整演示
 * ============================================================================
 *
 *  【一、AVL 树是什么】
 *  它首先是一棵二叉搜索树（BST），但额外要求：
 *        任意节点的左右子树高度差 ≤ 1（平衡因子的取值只能是 -1 / 0 / +1）
 *  一旦插入或删除破坏了这条要求，就用「旋转」把它修回来。
 *
 *  【二、四种失衡形态与对策】（insert / deleteNode 里逐一判断）
 *        LL（左左）：左孩子的左子树太高   → 一次右旋
 *        RR（右右）：右孩子的右子树太高   → 一次左旋
 *        LR（左右）：左孩子的右子树太高   → 先对左孩子左旋，再右旋自己
 *        RL（右左）：右孩子的左子树太高   → 先对右孩子右旋，再左旋自己
 *
 *  【三、和红黑树的区别】（可对照 red-black_tree.c 一起看）
 *        AVL    ：严格平衡（高度差 ≤ 1）→ 树更矮、查得更快；插入/删除旋转更多
 *        红黑树 ：近似平衡（最长路径 ≤ 2×最短）→ 旋转少，工程上更常用
 *  两者的旋转骨架几乎一样，但红黑树多了 parent 指针和颜色，
 *  代码量差不多翻倍 —— 从这里就能看出「简单」和「常用」不总是一回事。
 *
 *  【四、为什么要存 height 字段】
 *  节点里存的高度是「空间换时间」：旋转后 O(1) 就能读到子树高度，
 *  不必每次递归重算。这也是 updateHeight 在插入/删除路径上无处不在的原因。
 *  ★ 一旦 height 没更新干净，平衡因子就会算错，后面所有旋转判断跟着全错 ——
 *    所以本文件的 validateAVL 会专门核对「存的 height 是否等于真实高度」。
 *
 *  【五、文件结构】
 *    1. 类型定义         Node（含 height 字段）
 *    2. 高度与平衡因子   getHeight / getBalanceFactor / updateHeight
 *    3. 旋转             rightRotate / leftRotate
 *    4. 插入             insert（末尾按四种情况旋转）
 *    5. 删除             deleteNode / minValueNode
 *    6. 遍历             inorderTraversal / preorderTraversal
 *    7. 校验与打印       search / validateAVL / printTree / countNodes / destroyTree
 *                        （本节是为「能验证、能演示」而新增的，前面代码未改动）
 *    8. main 演示        插入 / 查找 / 删除 / 随机增删压力测试
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>    /* INT_MIN / INT_MAX，校验 BST 区间时当边界用 */

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

/* ========== 7. 查找、校验与打印（本节为「能验证、能演示」新增）========== */

/*
 * 查找：AVL 树首先是一棵 BST，所以查找逻辑和 bst.c 完全一样。
 * 不同之处在于「树高有保证」：AVL 的树高不超过约 1.44·log2(n)，
 * 所以查找稳定是 O(log n)，不会像普通 BST 那样退化成 O(n)。
 */
Node *search(Node *root, int data) {
    while (root != NULL) {
        if (data == root->data) return root;                    /* 命中 */
        root = (data < root->data) ? root->left : root->right;  /* 小走左，大走右 */
    }
    return NULL;                                                /* 走到空位 = 不存在 */
}

/* 节点总数 */
int countNodes(Node *root) {
    if (root == NULL) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

/*
 * AVL 校验：一次核对四件事
 *   ① BST 有序性：用「区间夹住整棵子树」的办法（和 bst.c 的 isBST 同思路）
 *   ② 节点里存的 height 是否等于真实高度   ← 最容易写错的一条
 *   ③ 平衡因子是否满足 |bf| ≤ 1            ← AVL 树的核心要求
 *   ④ 左右子树递归下去同样满足以上三条
 * 返回值 = 以 node 为根的真实高度；一旦发现违规就把 *ok 置为 false。
 */
static int checkAVL(Node *node, int lower, int upper, bool *ok) {
    if (node == NULL) return 0;

    if (node->data <= lower || node->data >= upper) {
        *ok = false;                                    /* ① 越界了 */
    }

    int lh = checkAVL(node->left,  lower, node->data, ok);
    int rh = checkAVL(node->right, node->data, upper, ok);

    if (node->height != (lh > rh ? lh : rh) + 1) {
        *ok = false;                                    /* ② height 漂了 */
    }
    if (lh - rh > 1 || rh - lh > 1) {
        *ok = false;                                    /* ③ 失衡了 */
    }

    return (lh > rh ? lh : rh) + 1;
}

bool validateAVL(Node *root) {
    bool ok = true;
    checkAVL(root, INT_MIN, INT_MAX, &ok);
    return ok;
}

/*
 * 打印树形：缩进表示层级，L / R 表示是父节点的左孩子还是右孩子，
 * 括号里给出该节点存的高度和平衡因子。
 * bf 到 ±1 就已经是临界值，所以这张图能直接看出「哪里快撑不住了」。
 */
void printTree(Node *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");

    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");

    printf("%d (h=%d, bf=%+d)\n", node->data, node->height, getBalanceFactor(node));

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* 递归释放整棵树：必须先释放两个孩子，最后释放自己 */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* 把中序结果收集到数组，用于独立检查「是否严格升序」 */
static int collectInorder(Node *node, int *out, int idx) {
    if (node == NULL) return idx;
    idx = collectInorder(node->left, out, idx);
    out[idx++] = node->data;
    return collectInorder(node->right, out, idx);
}

static bool inorderSorted(Node *root, int count) {
    int buf[2048];
    if (count > (int)(sizeof(buf) / sizeof(buf[0]))) return false;
    if (collectInorder(root, buf, 0) != count) return false;
    for (int i = 1; i < count; i++) {
        if (buf[i - 1] >= buf[i]) return false;
    }
    return true;
}

/* 小型伪随机数发生器：固定种子 → 每次运行结果完全一致，便于对比和排查 */
static unsigned int g_seed = 20260918u;

static unsigned int nextRand(void) {
    g_seed = g_seed * 1103515245u + 12345u;   /* 线性同余，无符号溢出即取模 */
    return (g_seed >> 16) & 0x7FFFu;
}

/* 洗牌（Fisher-Yates） */
static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(nextRand() % (unsigned int)(i + 1));
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* ============================ 8. main 演示 ============================ */

int main(void) {
    printf("==========================================================\n");
    printf("  AVL Tree demo\n");
    printf("==========================================================\n");
    printf("  Legend: h = stored height, bf = balance factor (left - right)\n\n");

    Node *root = NULL;

    /* ---------------- [1] 插入：旋转会不断发生 ---------------- */
    int keys[] = {10, 20, 30, 40, 50, 25};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    printf("[1] Insert: ");
    for (int i = 0; i < n; i++) printf("%d ", keys[i]);
    printf("\n");
    printf("    A plain BST would build a chain of height %d (see bst.c),\n", n);
    printf("    while AVL rotates right away to keep |bf| <= 1.\n\n");

    for (int i = 0; i < n; i++) {
        root = insert(root, keys[i]);
        printf("---- after inserting %d ----\n", keys[i]);
        printTree(root, 0, ' ');
        printf("nodes: %d    height: %d    valid AVL: %s\n\n",
               countNodes(root), getHeight(root), validateAVL(root) ? "YES" : "NO");
    }

    printf("    Inorder  : "); inorderTraversal(root);  printf("\n");
    printf("    Preorder : "); preorderTraversal(root); printf("\n\n");

    /* ---------------- [2] 查找 ---------------- */
    printf("[2] Search\n");
    int query[] = {25, 99, 10};
    for (int i = 0; i < 3; i++) {
        Node *found = search(root, query[i]);
        if (found != NULL) {
            printf("  %3d -> found (height %d, bf %+d)\n",
                   query[i], found->height, getBalanceFactor(found));
        } else {
            printf("  %3d -> not found\n", query[i]);
        }
    }
    printf("\n");

    /* ---------------- [3] 删除：每次删除后都可能触发旋转 ---------------- */
    printf("[3] Delete demo\n");
    printf("    Each step reports the shape of the node being removed,\n");
    printf("    then re-balances and re-validates the whole tree.\n\n");

    int delKeys[] = {30, 10, 50, 20, 25, 40, 999};
    int delCount = (int)(sizeof(delKeys) / sizeof(delKeys[0]));

    for (int i = 0; i < delCount; i++) {
        int key = delKeys[i];

        /* 先查这个键在不在，并数一数它有几个孩子（这样打印出的说明永远准确） */
        Node *z = search(root, key);
        if (z == NULL) {
            printf("---- delete %d : key not found ----\n", key);
        } else {
            int kids = (z->left != NULL) + (z->right != NULL);
            const char *kind = (kids == 0) ? "leaf"
                             : (kids == 1) ? "node with one child"
                                           : "node with two children";
            printf("---- delete %d : %s, bf %+d ----\n", key, kind, getBalanceFactor(z));
        }

        root = deleteNode(root, key);

        if (root == NULL) {
            printf("(empty tree)\n");
        } else {
            printTree(root, 0, ' ');
        }
        printf("nodes: %d    height: %d    valid AVL: %s\n",
               countNodes(root), getHeight(root), validateAVL(root) ? "YES" : "NO");
        printf("inorder: "); inorderTraversal(root); printf("\n\n");
    }

    destroyTree(root);
    root = NULL;

    /* ---------------- [4] 随机压力测试 ---------------- */
    printf("[4] Stress test: 300 random inserts + 300 random deletes\n");
    printf("    After every operation we verify BST ordering, stored heights\n");
    printf("    and the balance factor of every node.\n\n");

    enum { STRESS_N = 300 };
    int data[STRESS_N];
    for (int i = 0; i < STRESS_N; i++) data[i] = i + 1;

    Node *big = NULL;
    int steps = 0, pass = 0, fail = 0;
    bool sortedOK = true;

    /* 第一段：把 1..300 打乱顺序插进去 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        big = insert(big, data[i]);
        steps++;
        if (validateAVL(big)) pass++; else fail++;
        if (!inorderSorted(big, countNodes(big))) sortedOK = false;
    }
    printf("    after %d inserts : nodes = %d, height = %d\n",
           STRESS_N, countNodes(big), getHeight(big));
    printf("    (AVL keeps height near 1.44*log2(n); a plain BST is usually ~2x taller)\n\n");

    /* 第二段：再打乱一次顺序，把所有键逐个删光 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        int before = countNodes(big);
        big = deleteNode(big, data[i]);
        if (countNodes(big) != before - 1) {
            printf("    ERROR: deleting %d did not remove exactly one node\n", data[i]);
            fail++;
        }
        steps++;
        if (validateAVL(big)) pass++; else fail++;
        if (!inorderSorted(big, countNodes(big))) sortedOK = false;
    }

    /* 再删几个不存在的键，确认不会误删、也不会破坏结构 */
    for (int k = 5000; k < 5005; k++) {
        big = deleteNode(big, k);
        if (validateAVL(big)) pass++; else fail++;
    }

    printf("    operations validated : %d\n", steps);
    printf("    property check PASS  : %d\n", pass);
    printf("    property check FAIL  : %d\n", fail);
    printf("    inorder always sorted: %s\n", sortedOK ? "YES" : "NO");
    printf("    remaining nodes      : %d\n\n", countNodes(big));

    /* ---------------- [5] 释放 ---------------- */
    destroyTree(big);
    printf("[5] All trees have been freed\n");

    return 0;
}