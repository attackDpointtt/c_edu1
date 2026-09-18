/*
 * ============================================================================
 *  二叉树（Binary Tree）基础演示
 * ============================================================================
 *
 *  【一、什么是二叉树】
 *  每个节点最多有两个孩子：左孩子、右孩子。
 *  注意「最多」两个字 —— 可以只有左孩子，也可以一个孩子都没有（叶子）。
 *  二叉树不要求「左 < 右」，也没有任何平衡要求，是最朴素的一种树。
 *
 *  【二、它和后面那些树的关系】
 *        二叉树    ：只要求「每个节点最多两个孩子」            ← 本文件
 *        BST      ：再要求「左子树全小、右子树全大」           → bst.c
 *        AVL/红黑 ：再要求「树高被限制住」                     → avl_tree.c / red-black_tree.c
 *  也就是说，二叉树是所有这些结构的地基，后面的「树」都是在它身上加约束。
 *
 *  【三、几种树的表示法（本目录里的兄弟文件）】
 *        二叉链表（本文件）：每个节点两个指针 left / right，最多两个孩子
 *        孩子表示法        ：每个节点挂一个孩子链表，孩子数不限   → child_tree.c
 *        孩子兄弟表示法    ：左孩子 + 右兄弟，本质仍是二叉树     → child_sibling_tree.c
 *        双亲表示法        ：数组 + 父节点下标                   → parents_tree.c
 *  选哪种，取决于「你要频繁做哪种操作」：找孩子多？找父亲多？孩子数是否固定？
 *
 *  【四、文件结构】
 *    1. 类型定义与建节点   Node / createNode        ← 保留原始代码，只补注释
 *    2. 遍历               前序 / 中序 / 后序 / 层序
 *    3. 统计               节点数 / 高度 / 叶子数
 *    4. 其它操作           镜像反转 / 缩进打印 / 释放
 *    5. 自检工具           expectInt / seqEquals（把遍历结果和预期序列逐项比对）
 *    6. main 演示
 *
 *  【五、一个值得记住的小技巧】
 *  本文件的遍历函数把结果写进缓冲区（visit()）而不是直接 printf，
 *  这样同一份遍历代码既能打印给人看，又能拿去和预期序列比对做自检 ——
 *  「能自检的代码」比「看起来对的代码」可靠得多。
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

/*
 * 造一个孤立的新节点（左右孩子都是空的）。
 * 注意：malloc 出来的内存里是垃圾值，所以三个字段都必须显式赋值。
 */
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {              /* 分配失败要立刻退出，别拿着 NULL 往下写 */
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------
 * C 编译器自上而下单遍扫描，函数必须先声明后使用。
 */
void  preorder(Node *root);
void  inorder(Node *root);
void  postorder(Node *root);
void  levelOrder(Node *root);
int   countNodes(Node *root);
int   height(Node *root);
int   countLeaves(Node *root);
void  mirror(Node *root);
void  printTree(Node *node, int depth, char side);
void  destroyTree(Node *root);

/* ======================= 1. 自检小工具 ======================= */

/*
 * 遍历结果先写进这个缓冲区，而不是直接 printf。
 * 好处：同一份遍历代码既能打印，也能与「预期序列」逐项比对做自检。
 */
static int g_out[64];
static int g_out_n;

static void visit(int value) {
    if (g_out_n < (int)(sizeof(g_out) / sizeof(g_out[0]))) {
        g_out[g_out_n++] = value;
    }
}

static int g_pass = 0;
static int g_fail = 0;

/* 比对两个整数，打印结论并累计通过/失败数 */
static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-30s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-30s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

/* 比对整段遍历序列：长度一致且逐项相等才算通过 */
static void expectSeq(const char *what, const int *want, int n) {
    bool same = (g_out_n == n);
    for (int i = 0; same && i < n; i++) {
        if (g_out[i] != want[i]) same = false;
    }

    printf("  [%s] %-30s: ", same ? "ok  " : "FAIL", what);
    for (int i = 0; i < g_out_n; i++) printf("%d ", g_out[i]);
    if (!same) {
        printf("  (expected:");
        for (int i = 0; i < n; i++) printf(" %d", want[i]);
        printf(" )");
    }
    printf("\n");

    if (same) g_pass++; else g_fail++;
}

/* 把缓冲区里的遍历结果打印成一行 */
static void printSeq(const char *label) {
    printf("%-10s:", label);
    for (int i = 0; i < g_out_n; i++) printf(" %d", g_out[i]);
    printf("\n");
}

/* ============================ 2. 遍历 ============================ */

/*
 * 三种深度优先遍历的递归写法几乎一模一样，差别只在「printf 放哪一行」：
 *        前序：根 → 左 → 右        中序：左 → 根 → 右        后序：左 → 右 → 根
 * 把 visit 挪个位置，就是另一种遍历 —— 这一点值得反复体会。
 */
void preorder(Node *root) {
    if (root == NULL) return;
    visit(root->data);
    preorder(root->left);
    preorder(root->right);
}

/*
 * 中序对普通二叉树没有特殊含义（只有 BST 的中序才必然升序，见 bst.c）。
 */
void inorder(Node *root) {
    if (root == NULL) return;
    inorder(root->left);
    visit(root->data);
    inorder(root->right);
}

void postorder(Node *root) {
    if (root == NULL) return;
    postorder(root->left);
    postorder(root->right);
    visit(root->data);
}

/*
 * 层序（广度优先）：一层一层、从左到右。
 * 它没法用普通递归实现，必须借助队列（先进先出）：
 *        根入队 → 循环 { 出队并访问；把左右孩子依次入队 }
 * 这里用固定数组当队列，够演示用（不做动态扩容）。
 */
void levelOrder(Node *root) {
    if (root == NULL) return;

    Node *queue[256];
    int front = 0, rear = 0;

    queue[rear++] = root;
    while (front < rear) {
        Node *cur = queue[front++];
        visit(cur->data);
        if (cur->left  != NULL) queue[rear++] = cur->left;
        if (cur->right != NULL) queue[rear++] = cur->right;
    }
}

/* ============================ 3. 统计 ============================ */

/* 节点总数 = 1（自己）+ 左子树 + 右子树 */
int countNodes(Node *root) {
    if (root == NULL) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

/* 树高：空树 0，只有根节点为 1 */
int height(Node *root) {
    if (root == NULL) return 0;
    int lh = height(root->left);
    int rh = height(root->right);
    return (lh > rh ? lh : rh) + 1;
}

/* 叶子数：左右孩子都为空的节点 */
int countLeaves(Node *root) {
    if (root == NULL) return 0;
    if (root->left == NULL && root->right == NULL) return 1;
    return countLeaves(root->left) + countLeaves(root->right);
}

/* ======================= 4. 其它操作 ======================= */

/*
 * 镜像反转：把每个节点的左右孩子交换。
 * 递归下去交换每一层，整棵树就左右翻了个面。
 * 有趣的性质：镜像后的中序序列 = 原中序序列的逆序。
 */
void mirror(Node *root) {
    if (root == NULL) return;
    Node *tmp = root->left;
    root->left = root->right;
    root->right = tmp;
    mirror(root->left);
    mirror(root->right);
}

/*
 * 打印树形：缩进表示层级，L / R 表示是父节点的左孩子还是右孩子。
 * 全 ASCII 输出，不会乱码，也不用画连接线。
 */
void printTree(Node *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");
    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");
    printf("%d\n", node->data);

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* 递归释放：必须先释放两个孩子，最后释放自己（反过来就找不到孩子了） */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* ============================ 5. main 演示 ============================ */

/*
 * 手工搭一棵满二叉树（7 个节点，高 3，4 片叶子）：
 *
 *              1
 *           /     \
 *          2       3
 *         / \     / \
 *        4   5   6   7
 */
static Node *buildSample(void) {
    Node *n1 = createNode(1);
    Node *n2 = createNode(2);
    Node *n3 = createNode(3);
    Node *n4 = createNode(4);
    Node *n5 = createNode(5);
    Node *n6 = createNode(6);
    Node *n7 = createNode(7);

    n1->left = n2;  n1->right = n3;
    n2->left = n4;  n2->right = n5;
    n3->left = n6;  n3->right = n7;
    return n1;
}

int main(void) {
    printf("==========================================================\n");
    printf("  Binary Tree demo\n");
    printf("==========================================================\n\n");

    Node *root = buildSample();

    /* ---------------- [1] 树形与遍历 ---------------- */
    printf("[1] Shape of the tree (L = left child, R = right child)\n");
    printTree(root, 0, ' ');
    printf("\n");

    printf("[2] Traversals\n");
    g_out_n = 0; preorder(root);  printSeq("Preorder");
    g_out_n = 0; inorder(root);   printSeq("Inorder");
    g_out_n = 0; postorder(root); printSeq("Postorder");
    g_out_n = 0; levelOrder(root);printSeq("Level");
    printf("\n");

    /* ---------------- [3] 统计 ---------------- */
    printf("[3] Statistics\n");
    printf("  nodes = %d   height = %d   leaves = %d\n\n",
           countNodes(root), height(root), countLeaves(root));

    /* ---------------- [4] 镜像 ---------------- */
    printf("[4] Mirror the tree (swap every node's left and right child)\n");
    mirror(root);
    printTree(root, 0, ' ');
    printf("  inorder after mirroring is the reverse of the original one\n\n");

    /* ---------------- [5] 自检 ---------------- */
    printf("[5] Self checks\n");

    static const int wantPre[]  = {1, 3, 7, 6, 2, 5, 4};   /* 镜像后的前序 */
    static const int wantIn[]   = {7, 3, 6, 1, 5, 2, 4};   /* 镜像后的中序 */
    static const int wantPost[] = {7, 6, 3, 5, 4, 2, 1};   /* 镜像后的后序 */
    static const int wantLvl[]  = {1, 3, 2, 7, 6, 5, 4};   /* 镜像后的层序 */

    g_out_n = 0; preorder(root);   expectSeq("mirrored preorder",  wantPre,  7);
    g_out_n = 0; inorder(root);    expectSeq("mirrored inorder",   wantIn,   7);
    g_out_n = 0; postorder(root);  expectSeq("mirrored postorder", wantPost, 7);
    g_out_n = 0; levelOrder(root); expectSeq("mirrored level",     wantLvl,  7);

    expectInt("node count",   countNodes(root), 7);
    expectInt("height",       height(root),     3);
    expectInt("leaf count",   countLeaves(root), 4);
    expectInt("height(empty)", height(NULL),     0);

    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [6] 释放 ---------------- */
    destroyTree(root);
    printf("[6] The tree has been freed\n");

    return 0;
}
