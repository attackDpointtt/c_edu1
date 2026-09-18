/*
 * ============================================================================
 *  树的孩子表示法（孩子链表）完整演示
 * ============================================================================
 *
 *  【说明：这个文件原来的内容】
 *  原文件里的 Node 定义其实是**二叉树的节点**（left / right），和「孩子表示法」
 *  不是一回事 —— 应该是从 binary_tree.c 复制过来的。按「原有代码保留」的原则，
 *  这段定义原样留着，并在下面 [1] 段用它做**引子**：
 *        用 left/right 只能挂两个孩子，而现实中的树（目录、组织架构…）
 *        一个节点的孩子数是不固定的。
 *  真正的孩子表示法从第 2 节开始。
 *
 *  【一、孩子表示法长什么样】
 *  每个节点自己存数据，另外挂一条「孩子单链表」——孩子有几个就挂几个：
 *
 *        1 ──→ [2] ──→ [3] ──→ [4]
 *               │        │
 *               ↓        ↓
 *              [5]──→[6] [7]
 *
 *  结构体：  CNode { data, firstChild, nextChild }
 *            firstChild —— 孩子链表的头（没孩子就是 NULL）
 *            nextChild  —— 下一个兄弟（注意：这条链串的是「兄弟」，不是层次）
 *
 *  【二、和其它几种表示法比】
 *        二叉链表        ：left / right，最多两个孩子        → binary_tree.c
 *        孩子表示法      ：孩子链表，孩子数不限（本文件）
 *        孩子兄弟表示法  ：只留两个指针（左孩子 + 右兄弟）    → child_sibling_tree.c
 *        双亲表示法      ：数组 + 父下标，找父亲 O(1)         → parents_tree.c
 *  孩子表示法的特点是「往下走（找孩子）很方便，往上走（找父亲）要遍历整棵树」。
 *
 *  【三、文件结构】
 *    1. 原始定义与引子     Node / createNode（保留原代码）
 *    2. 孩子表示法         CNode / createCNode / addChild
 *    3. 遍历与统计         先根 / 后根 / 节点数 / 深度 / 某节点的孩子数
 *    4. 打印与释放         printTree / destroyTree
 *    5. 自检工具           expectInt / expectSeq
 *    6. main 演示
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============ 1. 原始定义（保留原代码）与它暴露的问题 ============ */

typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

/* 造一个二叉节点（左右孩子都是空的） */
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

/*
 * 引子：用 left/right 表示「节点 1 有三个孩子 2、3、4」。
 * 前两个孩子有地方放，第三个就无处安放了 ——
 * 这正是需要「孩子表示法」的原因。
 */
static void showBinaryLimit(void) {
    Node* n1 = createNode(1);
    Node* n2 = createNode(2);
    Node* n3 = createNode(3);

    n1->left = n2;
    n1->right = n3;

    printf("  node 1 can attach: left=%d  right=%d\n", n1->left->data, n1->right->data);
    printf("  the third child (4) has nowhere to go -- left/right are full.\n");
    printf("  a general tree needs another representation => see below.\n");

    free(n1); free(n2); free(n3);
}

/* ======================= 2. 孩子表示法 ======================= */

/*
 * 孩子表示法的节点：
 *   firstChild —— 我的第一个孩子（整条孩子链表的头）
 *   nextChild  —— 我的下一个兄弟（把自己串进父亲的孩子链表里）
 * 注意这两个指针的语义完全不同：一个是「往下」一层，一个是「往右」同层。
 */
typedef struct CNode {
    int data;
    struct CNode *firstChild;
    struct CNode *nextChild;
} CNode;

CNode *createCNode(int data) {
    CNode *node = (CNode *)malloc(sizeof(CNode));
    if (node == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->data = data;
    node->firstChild = NULL;
    node->nextChild = NULL;
    return node;
}

/*
 * 给 parent 添一个孩子 child。
 * 为了让孩子顺序和书写顺序一致，挂到孩子链表的**尾部**。
 * （两种插入一样对，取决于你需不需要「有序的孩子」。）
 */
void addChild(CNode *parent, CNode *child) {
    if (parent->firstChild == NULL) {
        parent->firstChild = child;
        return;
    }
    CNode *p = parent->firstChild;
    while (p->nextChild != NULL) {
        p = p->nextChild;
    }
    p->nextChild = child;
}

/* 某个节点有几个孩子？ */
int childCount(CNode *node) {
    int n = 0;
    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        n++;
    }
    return n;
}

/* ======================= 3. 遍历与统计 ======================= */

/*
 * 先根遍历（对应二叉树的先序）：先访问自己，再依次遍历每棵子树。
 * 注意内层是「孩子链表」的循环 —— 这是它与二叉树遍历最大的区别：
 * 二叉树写死两次递归（left / right），这里用一个 for 循环处理任意多个孩子。
 */
void preorderRootFirst(CNode *node, int *out, int *n) {
    if (node == NULL) return;
    out[(*n)++] = node->data;
    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        preorderRootFirst(c, out, n);
    }
}

/* 后根遍历（对应二叉树的后序）：先遍历每棵子树，最后访问自己 */
void postorderRootLast(CNode *node, int *out, int *n) {
    if (node == NULL) return;
    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        postorderRootLast(c, out, n);
    }
    out[(*n)++] = node->data;
}

/* 节点总数：1（自己）+ 每棵子树的节点数 */
int countNodes(CNode *node) {
    if (node == NULL) return 0;
    int total = 1;
    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        total += countNodes(c);
    }
    return total;
}

/* 深度（层数）：空树 0，只有根为 1；等于「最深的子树深度 + 1」 */
int depth(CNode *node) {
    if (node == NULL) return 0;
    int best = 0;
    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        int d = depth(c);
        if (d > best) best = d;
    }
    return best + 1;
}

/* ======================= 4. 打印与释放 ======================= */

/*
 * 缩进打印：每个孩子比父节点多缩进一级。
 * 因为孩子数不限，所以不用像二叉树那样标 L/R，缩进本身就表达了层级。
 */
void printTree(CNode *node, int depthLevel) {
    if (node == NULL) return;

    for (int i = 0; i < depthLevel; i++) printf("    ");
    printf("%d\n", node->data);

    for (CNode *c = node->firstChild; c != NULL; c = c->nextChild) {
        printTree(c, depthLevel + 1);
    }
}

/*
 * 释放整棵树：先递归释放所有孩子，最后释放自己。
 * 注意必须先保存 nextChild 再递归 —— 万一递归里把当前节点的内存改了，
 * 你就找不到下一个兄弟了。这里用 for 循环的 c = c->nextChild 是安全的：
 * 我们释放的是 c 指向的子树，c->nextChild 在释放前已经读出来了。
 */
void destroyTree(CNode *node) {
    if (node == NULL) return;
    CNode *c = node->firstChild;
    while (c != NULL) {
        CNode *next = c->nextChild;      /* ★ 先记下下一个兄弟 */
        destroyTree(c);
        c = next;
    }
    free(node);
}

/* ======================= 5. 自检工具 ======================= */

static int g_pass = 0;
static int g_fail = 0;

static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-28s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-28s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

static void expectSeq(const char *what, const int *got, int gotN, const int *want, int wantN) {
    bool same = (gotN == wantN);
    for (int i = 0; same && i < wantN; i++) {
        if (got[i] != want[i]) same = false;
    }
    printf("  [%s] %-28s:", same ? "ok  " : "FAIL", what);
    for (int i = 0; i < gotN; i++) printf(" %d", got[i]);
    if (!same) {
        printf("  (expected:");
        for (int i = 0; i < wantN; i++) printf(" %d", want[i]);
        printf(" )");
    }
    printf("\n");
    if (same) g_pass++; else g_fail++;
}

/* ============================ 6. main 演示 ============================ */

/*
 * 搭一棵「孩子数不固定」的树：
 *              1
 *          /   |   \
 *         2    3    4
 *        / \    \
 *       5   6    7
 * 先根：1 2 5 6 3 7 4      后根：5 6 2 7 3 4 1
 */
int main(void) {
    printf("==========================================================\n");
    printf("  General tree: child-list representation\n");
    printf("==========================================================\n\n");

    /* ---------------- [1] 先看二叉节点的局限 ---------------- */
    printf("[1] Why left/right is not enough (original code in this file)\n");
    showBinaryLimit();
    printf("\n");

    /* ---------------- [2] 建树 ---------------- */
    printf("[2] Build a tree with the child-list representation\n");
    CNode *n1 = createCNode(1), *n2 = createCNode(2), *n3 = createCNode(3), *n4 = createCNode(4);
    CNode *n5 = createCNode(5), *n6 = createCNode(6), *n7 = createCNode(7);

    addChild(n1, n2); addChild(n1, n3); addChild(n1, n4);   /* 1 有三个孩子 */
    addChild(n2, n5); addChild(n2, n6);
    addChild(n3, n7);

    printTree(n1, 0);
    printf("\n");

    /* ---------------- [3] 遍历 ---------------- */
    int preBuf[32], n = 0;
    int postBuf[32], m = 0;

    preorderRootFirst(n1, preBuf, &n);
    postorderRootLast(n1, postBuf, &m);

    printf("[3] Traversals\n");
    printf("  root-first (preorder-like) : "); for (int i = 0; i < n; i++) printf("%d ", preBuf[i]); printf("\n");
    printf("  root-last  (postorder-like): "); for (int i = 0; i < m; i++) printf("%d ", postBuf[i]); printf("\n\n");

    printf("[4] Statistics\n");
    printf("  node count = %d   depth = %d   children of 1 = %d\n\n",
           countNodes(n1), depth(n1), childCount(n1));

    /* ---------------- [5] 自检 ---------------- */
    printf("[5] Self checks\n");
    static const int wantPre[]  = {1, 2, 5, 6, 3, 7, 4};
    static const int wantPost[] = {5, 6, 2, 7, 3, 4, 1};

    expectSeq("root-first order",  preBuf,  n, wantPre,  7);
    expectSeq("root-last order",   postBuf, m, wantPost, 7);
    expectInt("node count",        countNodes(n1), 7);
    expectInt("depth",             depth(n1),      3);
    expectInt("children of node 1", childCount(n1), 3);   /* ★ 二叉树做不到 */
    expectInt("children of node 4", childCount(n4), 0);
    expectInt("depth(empty)",      depth(NULL),    0);
    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [6] 释放 ---------------- */
    destroyTree(n1);
    printf("[6] The tree has been freed\n");

    return 0;
}
