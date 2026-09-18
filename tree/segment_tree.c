/*
 * ============================================================================
 *  线段树（Segment Tree）完整演示
 * ============================================================================
 *
 *  【一、要解决什么问题】
 *  给一个数组，反复做两件事：
 *        ① 询问某个区间 [l, r] 的和
 *        ② 修改某个位置的值
 *  暴力做法：查询 O(n)、修改 O(1)。
 *  如果查询有上百万次，O(n) 就顶不住了 —— 线段树把**两者都压到 O(log n)**。
 *
 *  【二、它长什么样】
 *  把区间一层层对半切开，每个节点负责一段区间，并存这段区间的汇总值（这里是和）：
 *
 *                   [1..8] sum=36
 *                /                \
 *          [1..4] 16            [5..8] 20
 *          /      \             /      \
 *      [1..2] 8  [3..4] 8   [5..6] 10  [7..8] 10
 *       /   \      /   \      /   \      /   \
 *      5     3    7     1    8     2    6     4
 *
 *  查询时：
 *        区间与当前节点**完全无关** → 直接返回 0（剪枝，这是效率的来源）
 *        区间**完全覆盖**当前节点 → 直接返回节点存的 sum，不再往下走 ★
 *        部分重叠               → 分给左右孩子各查一次，结果相加
 *
 *  【三、最容易写错的地方】
 *  修改一个叶子之后，**沿途所有祖先的 sum 都必须重新算一遍**（俗称「上拉 pull up」）。
 *  漏掉这一步，查询结果会在某次修改后开始悄悄出错 —— 树不会崩，只是数据不对。
 *  本文件的压力测试专门抓这种错：每次操作都拿暴力法的结果对一遍。
 *
 *  【四、本文件实现的是「单点修改 + 区间查询」】
 *  如果是「区间修改 + 区间查询」，需要引入**懒标记（lazy tag）**：
 *  整段被覆盖时不急着往下推，先把「我欠孩子们一次修改」记在自己身上，
 *  等真的要访问孩子时再推下去。那是线段树的进阶话题，这里先把基础打牢。
 *
 *  【五、文件结构】
 *    1. 类型定义与建节点   SegmentTree / createSegmentTree  ← 保留原始代码，只补注释
 *    2. 建树               build（递归二分 + 上拉）
 *    3. 查询与修改         query / update
 *    4. 打印与释放         printTree / destroyTree
 *    5. 自检工具           expectInt + 与暴力法对拍
 *    6. main 演示（含 2000 次随机操作的对拍压力测试）
 *
 *  约定：数组下标用 1..N（更贴近课本写法）；输出提示一律英文，注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============ 1. 类型定义与建节点（保留原始代码，只补注释）============ */

/*
 * 线段树的节点。
 *   sum        —— 这段区间的汇总值（本文件存「和」；换成 max/min 就是 RMQ 问题）
 *   start,end  —— 这个节点负责的区间（闭区间，下标从 1 开始）
 *   left,right —— 左右孩子；start == end 时说明是叶子，两个指针都是 NULL
 */
typedef struct SegmentTree {
    int sum;
    int start;
    int end;
    struct SegmentTree *left;
    struct SegmentTree *right;
} SegmentTree;

SegmentTree *createSegmentTree(int start, int end) {
    SegmentTree *newNode = (SegmentTree *)malloc(sizeof(SegmentTree));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->start = start;
    newNode->end = end;
    newNode->sum = 0;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------ */
SegmentTree *build(const int *a, int start, int end);
int   query(SegmentTree *node, int l, int r);
void  update(SegmentTree *node, int idx, int value);
void  printTree(SegmentTree *node, int depth, char side);
void  destroyTree(SegmentTree *node);

/* ======================= 2. 建树 ======================= */

/*
 * 由数组 a[start..end] 建树，返回这棵子树的根。
 *        叶子（start == end）：sum 就是那一个元素
 *        内部节点           ：递归建好左右孩子，再把自己的 sum 上拉为两者之和
 * 建树复杂度 O(n)：一共 2n-1 个节点，每个节点只算一次。
 */
SegmentTree *build(const int *a, int start, int end) {
    SegmentTree *node = createSegmentTree(start, end);

    if (start == end) {                   /* 叶子：区间里只有一个元素 */
        node->sum = a[start];
        return node;
    }

    int mid = (start + end) / 2;
    node->left  = build(a, start, mid);       /* 左半段 */
    node->right = build(a, mid + 1, end);     /* 右半段 */

    node->sum = node->left->sum + node->right->sum;   /* ★ 上拉 */
    return node;
}

/* ======================= 3. 查询与修改 ======================= */

/*
 * 查询区间 [l, r] 的和。
 * 三种情况对应着三种处理，缺一不可：
 *        ① 完全不重叠 → 返回 0（这一支被剪掉，是效率的来源）
 *        ② 完全被覆盖 → 直接返回本节点存的 sum（不再往下递归）
 *        ③ 部分重叠   → 左右各查一次，结果相加
 * 每次下去区间都会对半缩小，所以最多 O(log n) 层。
 */
int query(SegmentTree *node, int l, int r) {
    if (node == NULL)                 return 0;
    if (r < node->start || node->end < l) return 0;              /* ① 无关 */
    if (l <= node->start && node->end <= r) return node->sum;    /* ② 全覆盖 */

    return query(node->left,  l, r) + query(node->right, l, r);  /* ③ 部分重叠 */
}

/*
 * 把下标 idx 的值改成 value。
 * 做法：递归找到那条唯一的路径（idx 只会落在左孩子或右孩子之一），
 *       改完叶子后**一路返回时把沿途每个节点的 sum 重新算一遍**。
 *
 * ★ 忘了最后那行上拉，是线段树最经典的 bug：
 *   树的结构没坏、下标也没错，只是有些区间和停留在修改前的旧值，
 *   而且只在你查「包含 idx 的区间」时才暴露出来。
 */
void update(SegmentTree *node, int idx, int value) {
    if (node->start == node->end) {          /* 叶子：正是要改的那个位置 */
        node->sum = value;
        return;
    }

    int mid = (node->start + node->end) / 2;
    if (idx <= mid) {
        update(node->left, idx, value);
    } else {
        update(node->right, idx, value);
    }

    node->sum = node->left->sum + node->right->sum;   /* ★ 上拉，千万别漏 */
}

/* ======================= 4. 打印与释放 ======================= */

/* 缩进打印：[起..止] sum=值，叶子额外标为 leaf */
void printTree(SegmentTree *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");
    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");

    printf("[%d..%d] sum=%d%s\n", node->start, node->end, node->sum,
           (node->start == node->end) ? "  (leaf)" : "");

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* 递归释放：先孩子后自己 */
void destroyTree(SegmentTree *node) {
    if (node == NULL) return;
    destroyTree(node->left);
    destroyTree(node->right);
    free(node);
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

/* 暴力法求区间和：O(n)，但绝对不容易写错 —— 拿它当「标准答案」给线段树对拍 */
static int bruteSum(const int *a, int l, int r) {
    int total = 0;
    for (int i = l; i <= r; i++) total += a[i];
    return total;
}

/* 小型伪随机数发生器：固定种子 → 每次运行结果一致，便于复现问题 */
static unsigned int g_seed = 20260918u;

static unsigned int nextRand(void) {
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}

/* ============================ 6. main 演示 ============================ */

#define N 8      /* 数组长度（下标 1..N） */

int main(void) {
    printf("==========================================================\n");
    printf("  Segment tree demo (point update + range sum)\n");
    printf("==========================================================\n\n");

    /* 下标从 1 开始，a[0] 空着不用 */
    static int a[N + 1] = {0, 5, 3, 7, 1, 8, 2, 6, 4};

    printf("[1] Array:");
    for (int i = 1; i <= N; i++) printf(" %d", a[i]);
    printf("\n\n");

    SegmentTree *root = build(a, 1, N);

    printf("[2] The tree\n");
    printTree(root, 0, ' ');
    printf("\n");

    /* ---------------- [3] 区间查询 ---------------- */
    printf("[3] Range queries\n");
    printf("  sum(2..5) = %d\n", query(root, 2, 5));
    printf("  sum(1..8) = %d\n", query(root, 1, 8));
    printf("  sum(4..4) = %d\n", query(root, 4, 4));
    printf("  sum(1..3) = %d\n\n", query(root, 1, 3));

    /* ---------------- [4] 单点修改 ---------------- */
    printf("[4] Update a[3] from %d to 10\n", a[3]);
    a[3] = 10;
    update(root, 3, 10);
    printf("  sum(2..5) after update = %d\n", query(root, 2, 5));
    printf("  sum(1..8) after update = %d\n\n", query(root, 1, 8));

    /* ---------------- [5] 与暴力法对拍的压力测试 ---------------- */
    printf("[5] Stress test: random queries and updates vs brute force\n");
    printf("    Every query compares the segment tree with a plain O(n) loop.\n\n");

    enum { OPS = 2000 };
    int cmpPass = 0, cmpFail = 0, updates = 0, queries = 0;

    for (int i = 0; i < OPS; i++) {
        if (nextRand() % 3 == 0) {
            /* ① 单点修改：先改真值数组，再改线段树 */
            int idx = 1 + (int)(nextRand() % N);
            int val = (int)(nextRand() % 100);
            a[idx] = val;
            update(root, idx, val);
            updates++;

            /* 改完之后，整段和必须仍然等于暴力结果 */
            if (root->sum != bruteSum(a, 1, N)) cmpFail++;
            else cmpPass++;
        } else {
            /* ② 区间查询：随机取 l <= r，两边各算一次 */
            int l = 1 + (int)(nextRand() % N);
            int r = 1 + (int)(nextRand() % N);
            if (l > r) { int t = l; l = r; r = t; }
            queries++;

            if (query(root, l, r) != bruteSum(a, l, r)) {
                cmpFail++;
            } else {
                cmpPass++;
            }
        }
    }

    printf("    operations : %d queries + %d updates = %d\n", queries, updates, OPS);
    printf("    agree with brute force : %d\n", cmpPass);
    printf("    mismatches             : %d\n\n", cmpFail);

    /* ---------------- [6] 自检 ---------------- */
    printf("[6] Self checks\n");
    expectInt("tree root sum == brute total", root->sum, bruteSum(a, 1, N));
    expectInt("sum(4..4) is the leaf itself", query(root, 4, 4), a[4]);
    expectInt("query range with l > r returns 0", query(root, 5, 3), 0);
    expectInt("stress mismatches", cmpFail, 0);
    expectInt("all stress comparisons passed", (cmpPass == OPS) ? 1 : 0, 1);

    printf("\n  simple checks: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [7] 释放 ---------------- */
    destroyTree(root);
    printf("[7] The segment tree has been freed\n");

    return 0;
}
