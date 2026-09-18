/*
 * ============================================================================
 *  树的孩子兄弟表示法（左孩子 / 右兄弟）完整演示
 * ============================================================================
 *
 *  【一、只用两个指针表示任意多孩子的树】
 *  关键洞察：一个节点其实只需要知道两件事 ——
 *        ① 我的第一个孩子是谁        → firstChild
 *        ② 我的下一个兄弟是谁        → nextSibling
 *  有了这两条线，整棵树就串起来了：
 *
 *        1 ─firstChild─→ 2 ─nextSibling─→ 3 ─nextSibling─→ 4
 *                        │                 │
 *                   firstChild        firstChild
 *                        ↓                 ↓
 *                        5 ─→ 6            7
 *
 *  【二、为什么说它「本质就是一棵二叉树」】
 *  把 firstChild 看成二叉树的 left、nextSibling 看成 right，那么：
 *
 *        ★ 树的【先根遍历】  == 这棵二叉树的【先序遍历】 ★
 *        ★ 树的【后根遍历】  == 这棵二叉树的【中序遍历】 ★
 *
 *  这两条恒等式不是巧合，而是「树 → 二叉树」转换的理论基础 ——
 *  任何树都能这样存成一棵二叉树，于是二叉树的算法（遍历、线索化…）
 *  全都可以直接搬到树上用。本文件把两条恒等式都让程序自己验证。
 *
 *  【三、和相邻表示法比】
 *        孩子表示法      ：孩子链表，找孩子快，找父亲只能遍历  → child_tree.c
 *        孩子兄弟表示法  ：只用两个指针，且天然是二叉树（本文件）
 *        双亲表示法      ：数组 + 父下标，找父亲 O(1)          → parents_tree.c
 *
 *  【四、文件结构】
 *    1. 类型定义与建节点   Node / createNode        ← 保留原始代码，只补注释
 *    2. 建树               addChild / childCount
 *    3. 树视角的遍历       先根 / 后根
 *    4. 二叉树视角的遍历   先序 / 中序（指针用同一套，只是换个看法）
 *    5. 统计与打印         节点数 / 深度 / 缩进打印
 *    6. 自检工具           expectInt / expectSeq
 *    7. main 演示 + 释放
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============ 1. 类型定义（保留原始代码，只补注释）============ */

/*
 * firstChild  —— 第一个孩子（相当于二叉树的 left）
 * nextSibling —— 下一个兄弟（相当于二叉树的 right）
 * 就靠这两个指针，孩子数不受限制。
 */
typedef struct Node {
    int data;
    struct Node* firstChild;
    struct Node* nextSibling;
} Node;

/* 造一个孤立的新节点（没有孩子，也没有兄弟） */
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->firstChild = NULL;
    newNode->nextSibling = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------ */
void addChild(Node *parent, Node *child);
int  childCount(Node *node);
void treePreorder(Node *node, int *out, int *n);
void treePostorder(Node *node, int *out, int *n);
void binaryPreorder(Node *node, int *out, int *n);
void binaryInorder(Node *node, int *out, int *n);
int  countNodes(Node *node);
int  depth(Node *node);
void printTree(Node *node, int level);
void destroyTree(Node *node);

/* ======================= 2. 建树 ======================= */

/*
 * 给 parent 添一个孩子。
 * 注意：孩子是挂在「兄弟链」的末尾，所以要沿着 nextSibling 走到头。
 * 链尾节点的 nextSibling 为 NULL —— 顺便，这也就是「最后一个孩子」的判据。
 */
void addChild(Node *parent, Node *child) {
    if (parent->firstChild == NULL) {
        parent->firstChild = child;
        return;
    }
    Node *p = parent->firstChild;
    while (p->nextSibling != NULL) {
        p = p->nextSibling;
    }
    p->nextSibling = child;
}

/* 数一个节点有几个孩子（沿 firstChild 的兄弟链走一遍） */
int childCount(Node *node) {
    int n = 0;
    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        n++;
    }
    return n;
}

/* ======================= 3. 树视角的遍历 ======================= */

/*
 * 先根遍历：先访问自己，再依次访问「我的第一个孩子」及其所有兄弟。
 * 内层 while 沿着兄弟链走 —— 这就是「处理任意多个孩子」的写法。
 */
void treePreorder(Node *node, int *out, int *n) {
    if (node == NULL) return;
    out[(*n)++] = node->data;
    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        treePreorder(c, out, n);
    }
}

/* 后根遍历：先依次访问所有孩子，最后访问自己 */
void treePostorder(Node *node, int *out, int *n) {
    if (node == NULL) return;
    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        treePostorder(c, out, n);
    }
    out[(*n)++] = node->data;
}

/* ======================= 4. 二叉树视角的遍历 ======================= */

/*
 * 下面两个函数和 binary_tree.c 里的先序 / 中序**逐字相同**，
 * 只是把 left 换成了 firstChild、right 换成了 nextSibling。
 * 这正说明：孩子兄弟表示法存出来的，本来就是一棵二叉树。
 */
void binaryPreorder(Node *node, int *out, int *n) {
    if (node == NULL) return;
    out[(*n)++] = node->data;              /* 根 */
    binaryPreorder(node->firstChild,  out, n);   /* 左 = 第一个孩子 */
    binaryPreorder(node->nextSibling, out, n);   /* 右 = 下一个兄弟 */
}

void binaryInorder(Node *node, int *out, int *n) {
    if (node == NULL) return;
    binaryInorder(node->firstChild,  out, n);    /* 左 */
    out[(*n)++] = node->data;                    /* 根 */
    binaryInorder(node->nextSibling, out, n);    /* 右 */
}

/* ======================= 5. 统计与打印 ======================= */

/* 节点总数：1 + 每个孩子的子树大小 */
int countNodes(Node *node) {
    if (node == NULL) return 0;
    int total = 1;
    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        total += countNodes(c);
    }
    return total;
}

/* 深度：空树 0，最深的子树 + 1 */
int depth(Node *node) {
    if (node == NULL) return 0;
    int best = 0;
    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        int d = depth(c);
        if (d > best) best = d;
    }
    return best + 1;
}

/* 缩进打印：只沿 firstChild 往下，靠缩进表达层级 */
void printTree(Node *node, int level) {
    if (node == NULL) return;

    for (int i = 0; i < level; i++) printf("    ");
    printf("%d\n", node->data);

    for (Node *c = node->firstChild; c != NULL; c = c->nextSibling) {
        printTree(c, level + 1);
    }
}

/*
 * 释放整棵树：先释放每个孩子（含其兄弟链），最后释放自己。
 * 注意先在循环里递归完所有兄弟，再 free 自己。
 */
void destroyTree(Node *node) {
    if (node == NULL) return;
    Node *c = node->firstChild;
    while (c != NULL) {
        Node *next = c->nextSibling;      /* ★ 先记下下一个兄弟 */
        destroyTree(c);
        c = next;
    }
    free(node);
}

/* ======================= 6. 自检工具 ======================= */

static int g_pass = 0;
static int g_fail = 0;

static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-30s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-30s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

static void expectSeq(const char *what, const int *got, int gotN, const int *want, int wantN) {
    bool same = (gotN == wantN);
    for (int i = 0; same && i < wantN; i++) {
        if (got[i] != want[i]) same = false;
    }
    printf("  [%s] %-30s:", same ? "ok  " : "FAIL", what);
    for (int i = 0; i < gotN; i++) printf(" %d", got[i]);
    if (!same) {
        printf("  (expected:");
        for (int i = 0; i < wantN; i++) printf(" %d", want[i]);
        printf(" )");
    }
    printf("\n");
    if (same) g_pass++; else g_fail++;
}

/* ============================ 7. main 演示 ============================ */

/*
 * 搭一棵树：
 *              1
 *          /   |   \
 *         2    3    4
 *        / \    \
 *       5   6    7
 * 树的先根：1 2 5 6 3 7 4      树的后根：5 6 2 7 3 4 1
 * 而它作为二叉树的先序 = 先根，中序 = 后根（下面亲自验证）
 */
static int g_pre[32],  g_preN;
static int g_post[32], g_postN;
static int g_biPre[32], g_biPreN;
static int g_biIn[32],  g_biInN;

int main(void) {
    printf("==========================================================\n");
    printf("  Tree as binary tree: first-child / next-sibling\n");
    printf("==========================================================\n\n");

    Node *n1 = createNode(1), *n2 = createNode(2), *n3 = createNode(3), *n4 = createNode(4);
    Node *n5 = createNode(5), *n6 = createNode(6), *n7 = createNode(7);

    addChild(n1, n2); addChild(n1, n3); addChild(n1, n4);
    addChild(n2, n5); addChild(n2, n6);
    addChild(n3, n7);

    printf("[1] The tree\n");
    printTree(n1, 0);
    printf("\n");

    /* ---------------- [2] 两种视角的遍历 ---------------- */
    printf("[2] Same memory, two points of view\n");
    treePreorder(n1, g_pre, &g_preN);
    treePostorder(n1, g_post, &g_postN);
    binaryPreorder(n1, g_biPre, &g_biPreN);
    binaryInorder(n1, g_biIn, &g_biInN);

    printf("  as a TREE      : root-first  ="); for (int i = 0; i < g_preN;  i++) printf(" %d", g_pre[i]);  printf("\n");
    printf("  as a TREE      : root-last   ="); for (int i = 0; i < g_postN; i++) printf(" %d", g_post[i]); printf("\n");
    printf("  as a BINARY TREE: preorder   ="); for (int i = 0; i < g_biPreN; i++) printf(" %d", g_biPre[i]); printf("\n");
    printf("  as a BINARY TREE: inorder    ="); for (int i = 0; i < g_biInN;  i++) printf(" %d", g_biIn[i]);  printf("\n\n");

    printf("[3] Statistics\n");
    printf("  nodes = %d   depth = %d   children of 1 = %d\n\n",
           countNodes(n1), depth(n1), childCount(n1));

    /* ---------------- [4] 自检 ---------------- */
    printf("[4] Self checks\n");
    static const int wantPre[]  = {1, 2, 5, 6, 3, 7, 4};
    static const int wantPost[] = {5, 6, 2, 7, 3, 4, 1};

    expectSeq("tree root-first",      g_pre,   g_preN,  wantPre,  7);
    expectSeq("tree root-last",       g_post,  g_postN, wantPost, 7);
    expectSeq("binary preorder",      g_biPre, g_biPreN, wantPre,  7);
    expectSeq("binary inorder",       g_biIn,  g_biInN,  wantPost, 7);

    /* 两条恒等式的直接断言：树的先根 == 二叉树的先序，树的后根 == 二叉树的中序 */
    expectInt("tree-preorder == bin-preorder", (g_preN == g_biPreN), 1);
    expectInt("tree-postorder == bin-inorder", (g_postN == g_biInN), 1);

    expectInt("node count", countNodes(n1), 7);
    expectInt("depth",      depth(n1),      3);
    expectInt("children of node 1", childCount(n1), 3);
    expectInt("depth(empty)", depth(NULL), 0);
    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [5] 释放 ---------------- */
    destroyTree(n1);
    printf("[5] The tree has been freed\n");

    return 0;
}
