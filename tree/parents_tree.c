/*
 * ============================================================================
 *  树的双亲表示法（数组 + 父节点下标）完整演示
 * ============================================================================
 *
 *  【说明：这个文件原来的内容】
 *  原文件里的 Node 是「二叉链表 + parent 指针」（也叫三叉链表），和「双亲表示法」
 *  并不是一回事。按「原有代码保留」的原则，这段定义原样留着，并在下面第 1 节
 *  用它做**对照实现**：同一棵树，两种存法，各做一遍「求深度、求到根的路径」，
 *  再让程序自己比对两者结果是否一致。
 *
 *  【一、双亲表示法长什么样】
 *  就是一张表：每个节点只记「我是谁」和「我父亲是谁（下标）」。
 *
 *        下标  data  parent          对应的树：
 *         0     1     -1        (根)      1
 *         1     2      0               /   \
 *         2     3      0              2     3
 *         3     4      1             / \   /
 *         4     5      1            4   5 6
 *         5     6      2
 *
 *  【二、优缺点（选它之前先想清楚要做什么操作）】
 *        ✅ 找父亲 O(1)：顺着 parent 下标一跳就到
 *        ✅ 没有指针、结构紧凑、连续内存对 CPU 缓存友好，容易存进文件
 *        ✅ 孩子数不受限制（表里天然如此）
 *        ❌ 找孩子要 O(n)：只能把整张表扫一遍看谁的 parent 是自己
 *        ❌ 孩子之间没有顺序（想区分「第几个孩子」得另外记）
 *        ❌ 删除节点麻烦：后继节点的下标关系要维护（或用「假删除」标记）
 *  典型用途：并查集（Union-Find）用的就是这套表示法，只留 parent 数组。
 *
 *  【三、和相邻表示法比】
 *        孩子表示法      ：孩子链表，找孩子快        → child_tree.c
 *        孩子兄弟表示法  ：两个指针，天然是二叉树    → child_sibling_tree.c
 *        双亲表示法      ：数组 + 父下标（本文件）
 *        三叉链表        ：指针 + parent 指针（本文件第 1 节的对照实现）
 *
 *  【四、文件结构】
 *    1. 原始定义与对照     Node / createNode（三叉链表，保留原代码）
 *    2. 双亲表示法         PNode / PTree + 建表
 *    3. 常用操作           找父亲 / 找孩子 / 求深度 / 求根 / 到根的路径
 *    4. 打印               表 + 树形
 *    5. 自检工具           expectInt / expectSeq
 *    6. main 演示（两法结果互相印证）
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ====== 1. 原始定义（保留原代码）：三叉链表 ====== */

/*
 * 二叉节点 + parent 指针。
 * parent 的好处是「向上走」不用重新搜索：任意节点都能 O(1) 找到父亲。
 * 代价是每个节点多一个指针（8 字节），而且仍然只能有两个孩子。
 */
typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
    struct Node* parent;
} Node;

Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->parent = NULL;
    return newNode;
}

/* 沿 parent 往上数，求一个节点在第几层（根是第 1 层） */
int chainDepth(Node *node) {
    int d = 0;
    for (Node *p = node; p != NULL; p = p->parent) {
        d++;
    }
    return d;
}

/* 从 node 一路往上走到根，把路径写进 out，返回节点个数 */
int chainPathToRoot(Node *node, int *out) {
    int n = 0;
    for (Node *p = node; p != NULL; p = p->parent) {
        out[n++] = p->data;
    }
    return n;
}

/* 释放三叉链表：先释放孩子，再释放自己 */
void chainDestroy(Node *node) {
    if (node == NULL) return;
    chainDestroy(node->left);
    chainDestroy(node->right);
    free(node);
}

/* ======================= 2. 双亲表示法 ======================= */

#define MAX_NODES 64

/* 表里的一行：数据 + 父节点下标（根记 -1，表示「没有父亲」） */
typedef struct {
    int data;
    int parent;
} PNode;

/*
 * 整棵树 = 一张表 + 当前用了多少行。
 * 注意这里**没有指针**：父子关系全靠下标表达。
 */
typedef struct {
    PNode nodes[MAX_NODES];
    int   count;
} PTree;

void pInit(PTree *t) {
    t->count = 0;
}

/*
 * 往表尾加一个节点，返回它的下标。
 * parent 参数是父节点**在表中的下标**（根传 -1）。
 * 加完返回下标 —— 因为后面所有操作都要用它来找人。
 */
int pAdd(PTree *t, int data, int parent) {
    if (t->count >= MAX_NODES) {
        printf("table is full\n");
        exit(EXIT_FAILURE);
    }
    int idx = t->count++;
    t->nodes[idx].data = data;
    t->nodes[idx].parent = parent;
    return idx;
}

/* ======================= 3. 常用操作 ======================= */

/* 找父亲：O(1)，直接查表 */
int pParentOf(PTree *t, int idx) {
    return t->nodes[idx].parent;
}

/*
 * 找孩子：O(n) —— 把整张表扫一遍，看谁的 parent 指向 idx。
 * 这就是双亲表示法最大的短板，也是它常常要配合「孩子链表」一起用的原因。
 */
int pChildrenOf(PTree *t, int idx, int *out) {
    int n = 0;
    for (int i = 0; i < t->count; i++) {
        if (t->nodes[i].parent == idx) {
            out[n++] = i;
        }
    }
    return n;
}

/* 求深度（层数）：从自己出发，顺着 parent 一路往上数到根 */
int pDepthOf(PTree *t, int idx) {
    int d = 0;
    for (int i = idx; i != -1; i = t->nodes[i].parent) {
        d++;
    }
    return d;
}

/* 求根：一路向上走到 parent == -1 的那个节点 */
int pRootOf(PTree *t, int idx) {
    int i = idx;
    while (t->nodes[i].parent != -1) {
        i = t->nodes[i].parent;
    }
    return i;
}

/* 从 idx 到根的路径（自下而上），返回节点个数 */
int pPathToRoot(PTree *t, int idx, int *out) {
    int n = 0;
    for (int i = idx; i != -1; i = t->nodes[i].parent) {
        out[n++] = t->nodes[i].data;
    }
    return n;
}

/* ======================= 4. 打印 ======================= */

/* 把整张表打出来 —— 双亲表示法的「结构」就在这张表里，非常直观 */
void pPrintTable(PTree *t) {
    printf("  index  data  parent\n");
    for (int i = 0; i < t->count; i++) {
        printf("  %5d  %4d  %6d%s\n", i, t->nodes[i].data, t->nodes[i].parent,
               t->nodes[i].parent == -1 ? "   <- root" : "");
    }
}

/* 缩进打印树形：从 idx 出发，递归找它的孩子（每层都要扫表） */
void pPrintTree(PTree *t, int idx, int level) {
    for (int i = 0; i < level; i++) printf("    ");
    printf("%d\n", t->nodes[idx].data);

    int kids[MAX_NODES];
    int n = pChildrenOf(t, idx, kids);
    for (int i = 0; i < n; i++) {
        pPrintTree(t, kids[i], level + 1);
    }
}

/* ======================= 5. 自检工具 ======================= */

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

/* ============================ 6. main 演示 ============================ */

/*
 * 目标树（6 个节点，高 3）：
 *              1
 *            /   \
 *           2     3
 *          / \   /
 *         4   5 6
 */
int main(void) {
    printf("==========================================================\n");
    printf("  Tree as a parent-index array\n");
    printf("==========================================================\n\n");

    /* ---------------- [1] 建表 ---------------- */
    PTree t;
    pInit(&t);

    int i1 = pAdd(&t, 1, -1);      /* 根：下标 0，没有父亲 */
    int i2 = pAdd(&t, 2, i1);      /* 2 的父亲是 1 */
    int i3 = pAdd(&t, 3, i1);
    int i4 = pAdd(&t, 4, i2);
    int i5 = pAdd(&t, 5, i2);
    int i6 = pAdd(&t, 6, i3);

    printf("[1] The table itself is the tree\n");
    pPrintTable(&t);
    printf("\n");

    printf("[2] Shape recovered from the table\n");
    pPrintTree(&t, i1, 0);
    printf("\n");

    /* ---------------- [3] 各种操作 ---------------- */
    printf("[3] Operations\n");
    printf("  root of node 5        : %d\n", t.nodes[pRootOf(&t, i5)].data);
    printf("  depth of node 4       : %d\n", pDepthOf(&t, i4));
    printf("  depth of root         : %d\n", pDepthOf(&t, i1));

    int kids[MAX_NODES];
    int kn = pChildrenOf(&t, i2, kids);
    printf("  children of node 2    : ");
    for (int i = 0; i < kn; i++) printf("%d ", t.nodes[kids[i]].data);
    printf("(%d of them)\n", kn);

    int path[MAX_NODES];
    int pn = pPathToRoot(&t, i4, path);
    printf("  node 4 -> root        : ");
    for (int i = 0; i < pn; i++) printf("%d ", path[i]);
    printf("\n\n");

    /* ---------------- [4] 双亲表示法自检 ---------------- */
    printf("[4] Self checks (parent-index array)\n");
    static const int wantPath[] = {4, 2, 1};
    expectSeq("path 4 -> root",  path, pn, wantPath, 3);
    expectInt("root of node 5",  t.nodes[pRootOf(&t, i5)].data, 1);
    expectInt("depth of node 4", pDepthOf(&t, i4), 3);
    expectInt("depth of root",   pDepthOf(&t, i1), 1);
    expectInt("children of 2",   kn, 2);
    expectInt("parent of 6",     t.nodes[pParentOf(&t, i6)].data, 3);
    expectInt("node count",      t.count, 6);
    printf("\n");

    /* ---------------- [5] 用原始定义（三叉链表）做同一件事 ---------------- */
    printf("[5] Cross check: same tree stored as a parent-pointer linked structure\n");
    Node *c1 = createNode(1), *c2 = createNode(2), *c3 = createNode(3);
    Node *c4 = createNode(4), *c5 = createNode(5), *c6 = createNode(6);

    /* 连好 left/right，并**别忘了**连 parent */
    c1->left = c2;  c2->parent = c1;
    c1->right = c3; c3->parent = c1;
    c2->left = c4;  c4->parent = c2;
    c2->right = c5; c5->parent = c2;
    c3->left = c6;  c6->parent = c3;

    int linked[6];
    int ln = chainPathToRoot(c5, linked);          /* 5 号节点到根 */
    printf("  linked: depth of node 5 = %d\n", chainDepth(c5));
    printf("  linked: node 5 -> root  = ");
    for (int i = 0; i < ln; i++) printf("%d ", linked[i]);
    printf("\n");

    /* 两法对同一棵树求深度，结果必须一致 */
    int sameAll = 1;
    Node *arr[6] = {c1, c2, c3, c4, c5, c6};
    int   idx[6] = {i1, i2, i3, i4, i5, i6};
    for (int i = 0; i < 6; i++) {
        if (chainDepth(arr[i]) != pDepthOf(&t, idx[i])) sameAll = 0;
    }

    printf("\n[6] Self checks (cross check)\n");
    static const int wantLinked[] = {5, 2, 1};
    expectSeq("linked path 5 -> root", linked, ln, wantLinked, 3);
    expectInt("depths agree in all 6 nodes", sameAll, 1);
    expectInt("linked depth of node 5", chainDepth(c5), 3);
    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [7] 释放 ---------------- */
    chainDestroy(c1);
    /* 双亲表示法是数组，随结构体一起消失，不需要逐个 free —— 这也是它的优点 */
    printf("[7] Freed (the linked one recursively; the array needs no free at all)\n");

    return 0;
}
