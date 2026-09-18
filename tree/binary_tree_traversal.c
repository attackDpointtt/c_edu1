/*
 * ============================================================================
 *  二叉树遍历专题：递归 vs 非递归
 * ============================================================================
 *
 *  【一、遍历是什么】
 *  把树上每个节点都访问一次。按「访问根的时机」分成三种深度优先遍历：
 *        前序：根 → 左 → 右
 *        中序：左 → 根 → 右
 *        后序：左 → 右 → 根
 *  再加一种按层的广度优先遍历（层序），它必须借助队列，不能靠递归。
 *
 *  【二、递归是「系统帮你压栈」】
 *  递归写法之所以短，是因为编译器把「返回地址 + 局部变量」自动压进了调用栈。
 *  本文件把同一件事用「显式栈」再做一遍，你会看到：
 *        - 前序：栈里先压右、再压左（后压的先弹，所以先访问左）
 *        - 中序：一路向左压栈，弹出即访问，再转向右子树
 *        - 后序：最麻烦 —— 这里用「先走 根→右→左，再把结果倒过来」的等价技巧
 *  两者的输出必须一模一样，本文件会用程序自己比对（而不是靠肉眼）。
 *
 *  【三、为什么要学非递归】
 *        递归：代码短、可读；但栈深度受系统限制，树深 10 万就可能爆栈
 *        非递归：多写几行，但栈大小自己说了算，且能随时暂停 / 保存状态
 *  工程上还常用「Morris 遍历」做到 O(1) 额外空间（本文件不展开）。
 *
 *  【四、遍历序列能干什么】
 *  前序 + 中序 可以**唯一确定**一棵二叉树，本文件最后会用它把树重建出来验证。
 *  但「前序 + 后序」不行 —— 当一个节点只有一个孩子时，无法判断它是左还是右。
 *
 *  【五、文件结构】
 *    1. 类型定义与建节点   Node / createNode（原文件缺，补齐以便独立编译）
 *    2. 递归遍历           前序 / 中序 / 后序（保留原函数，只把 printf 换成 visit）
 *    3. 非递归遍历         用显式栈实现同样三种 + 层序用队列
 *    4. 从遍历序列重建树   前序 + 中序 → rebuild
 *    5. 自检工具           visit / expectSeq（两种实现结果逐项比对）
 *    6. main 演示
 *
 *  【关于这次改动的一点说明】
 *  原来三个递归函数里是直接 printf 的。为了让「递归版」和「非递归版」的输出
 *  能被程序自动比对，把它们改成了调用 visit()：visit 负责记录（也顺便打印）。
 *  遍历逻辑本身一个字没动。
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* 假定节点结构与 createNode 与 binary_tree.c 一致（原文件注释即如此说明） */
typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

/* 原文件没有 createNode，补上以便本文件能独立编译、独立演示 */
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

/* ------------------------------ 前向声明 ------------------------------ */
void inorderTraversal(Node* root);
void preorderTraversal(Node* root);
void postorderTraversal(Node* root);
void preorderIter(Node *root);
void inorderIter(Node *root);
void postorderIter(Node *root);
void levelOrder(Node *root);
void printTree(Node *node, int depth, char side);
void destroyTree(Node *root);

/* ======================= 1. 记录与自检 ======================= */

/*
 * 所有遍历都通过 visit 输出。它只负责「记录」到缓冲区，
 * 打印交给 printSeq —— 这样「递归版」和「非递归版」走的是同一条出口，
 * 既能打印给人看，也能拿去和预期序列做自动比对。
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

/* 打印本轮记录下来的遍历结果 */
static void printSeq(const char *label) {
    printf("  %-10s: ", label);
    for (int i = 0; i < g_out_n; i++) printf("%d ", g_out[i]);
    printf("\n");
}

/* 比对整段序列：长度一致且逐项相等才算通过 */
static void expectSeq(const char *what, const int *want, int n) {
    bool same = (g_out_n == n);
    for (int i = 0; same && i < n; i++) {
        if (g_out[i] != want[i]) same = false;
    }
    printf("  [%s] %-22s -> ", same ? "ok  " : "FAIL", what);
    for (int i = 0; i < g_out_n; i++) printf("%d ", g_out[i]);
    if (!same) {
        printf(" (expected:");
        for (int i = 0; i < n; i++) printf(" %d", want[i]);
        printf(" )");
    }
    printf("\n");
    if (same) g_pass++; else g_fail++;
}

/* ======================= 2. 递归遍历（保留原函数） ======================= */

// 中序遍历：左 → 根 → 右
/*
 * 三种递归遍历的代码形状完全一样，差别只在「visit 放在第几行」。
 */
void inorderTraversal(Node* root) {
    if (root != NULL) {
        inorderTraversal(root->left);
        visit(root->data);            /* 中间访问根 → 中序 */
        inorderTraversal(root->right);
    }
}

// 前序遍历：根 → 左 → 右
void preorderTraversal(Node* root) {
    if (root != NULL) {
        visit(root->data);            /* 最先访问根 → 前序 */
        preorderTraversal(root->left);
        preorderTraversal(root->right);
    }
}

// 后序遍历：左 → 右 → 根
void postorderTraversal(Node* root) {
    if (root != NULL) {
        postorderTraversal(root->left);
        postorderTraversal(root->right);
        visit(root->data);            /* 最后访问根 → 后序 */
    }
}

/* ======================= 3. 非递归遍历 ======================= */

/*
 * 非递归前序：用一个栈模拟递归。
 * 关键：弹出即访问；然后把「右孩子先压、左孩子后压」，
 *       这样下一轮弹出的就是左孩子 —— 正好符合「根 左 右」。
 */
void preorderIter(Node *root) {
    if (root == NULL) return;

    Node *stack[256];
    int top = 0;

    stack[top++] = root;
    while (top > 0) {
        Node *cur = stack[--top];
        visit(cur->data);
        if (cur->right != NULL) stack[top++] = cur->right;   /* 先压右 */
        if (cur->left  != NULL) stack[top++] = cur->left;    /* 后压左 → 先弹左 */
    }
}

/*
 * 非递归中序：
 *   ① 从当前节点一路向左，沿途全部压栈（这些节点都还没被访问）
 *   ② 弹出栈顶 —— 它就是「最左的、还没访问的」节点，访问它
 *   ③ 转向它的右子树，重复 ①
 * 循环条件是「栈不空 或 当前节点非空」。
 */
void inorderIter(Node *root) {
    Node *stack[256];
    int top = 0;
    Node *cur = root;

    while (cur != NULL || top > 0) {
        while (cur != NULL) {            /* ① 一路向左压栈 */
            stack[top++] = cur;
            cur = cur->left;
        }
        cur = stack[--top];              /* ② 弹出并访问 */
        visit(cur->data);
        cur = cur->right;                /* ③ 转右子树 */
    }
}

/*
 * 非递归后序：用「前序的镜像」取巧。
 * 先按「根 → 右 → 左」走一趟（把前序代码里左右压栈顺序对调即可），
 * 把结果存进数组，最后倒着输出 —— 倒过来的「根 右 左」正是「左 右 根」。
 * 相比「记录 lastVisited 指针」那套写法，这个版本好懂得多。
 */
void postorderIter(Node *root) {
    if (root == NULL) return;

    Node *stack[256];
    int top = 0;
    int tmp[256];
    int n = 0;

    stack[top++] = root;
    while (top > 0) {
        Node *cur = stack[--top];
        tmp[n++] = cur->data;                    /* 先记下「根 右 左」 */
        if (cur->left  != NULL) stack[top++] = cur->left;    /* 先压左 */
        if (cur->right != NULL) stack[top++] = cur->right;   /* 后压右 → 先弹右 */
    }
    for (int i = n - 1; i >= 0; i--) {           /* 倒着输出 */
        visit(tmp[i]);
    }
}

/*
 * 层序：必须用队列（先进先出）。
 * 根入队 → 循环 { 出队并访问；左右孩子依次入队 }。
 * 它没法用递归实现：递归是栈（后进先出），而「一层一层」需要先进先出。
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

/* ======================= 4. 由遍历序列重建树 ======================= */

/*
 * 由「前序 + 中序」重建二叉树。原理三步：
 *   ① 前序的第一个元素一定是整棵树的根；
 *   ② 拿着这个根去中序里找位置 k —— 中序里左边 k 个是左子树、右边是右子树；
 *   ③ 用 k 反过来切分前序，递归重建左右子树。
 * 递归边界是 n <= 0（空子树）。
 *
 * 为什么「前序 + 后序」不行？设想只有一个孩子的节点 1，孩子是 2：
 * 前序都是 [1,2]，后序都是 [2,1]，无法区分 2 是左孩子还是右孩子。
 */
Node *rebuild(const int *pre, const int *in, int n) {
    if (n <= 0) return NULL;

    int rootVal = pre[0];
    int k = -1;
    for (int i = 0; i < n; i++) {
        if (in[i] == rootVal) { k = i; break; }
    }
    if (k < 0) return NULL;              /* 两个序列对不上，输入非法 */

    Node *root = createNode(rootVal);
    root->left  = rebuild(pre + 1,     in,         k);           /* 左子树 */
    root->right = rebuild(pre + 1 + k, in + k + 1, n - k - 1);   /* 右子树 */
    return root;
}

/* ======================= 5. 打印与释放 ======================= */

/* 缩进打印树形，L / R 表示是父节点的左孩子还是右孩子 */
void printTree(Node *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");
    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");
    printf("%d\n", node->data);

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* 递归释放：先释放两个孩子，最后释放自己 */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* ============================ 6. main 演示 ============================ */

/*
 * 搭一棵 7 节点二叉树：
 *              1
 *           /     \
 *          2       3
 *         / \     / \
 *        4   5   6   7
 * 它的四种遍历序列（下面自检要用）：
 *        前序 1 2 4 5 3 6 7      中序 4 2 5 1 6 3 7
 *        后序 4 5 2 6 7 3 1      层序 1 2 3 4 5 6 7
 */
static Node *buildSample(void) {
    Node *n1 = createNode(1), *n2 = createNode(2), *n3 = createNode(3);
    Node *n4 = createNode(4), *n5 = createNode(5);
    Node *n6 = createNode(6), *n7 = createNode(7);

    n1->left = n2; n1->right = n3;
    n2->left = n4; n2->right = n5;
    n3->left = n6; n3->right = n7;
    return n1;
}

int main(void) {
    printf("==========================================================\n");
    printf("  Binary tree traversal: recursion vs explicit stack\n");
    printf("==========================================================\n\n");

    Node *root = buildSample();

    printf("[1] The tree\n");
    printTree(root, 0, ' ');
    printf("\n");

    /* 四种遍历的预期序列（供自检用） */
    static const int wantPre[]  = {1, 2, 4, 5, 3, 6, 7};
    static const int wantIn[]   = {4, 2, 5, 1, 6, 3, 7};
    static const int wantPost[] = {4, 5, 2, 6, 7, 3, 1};
    static const int wantLvl[]  = {1, 2, 3, 4, 5, 6, 7};

    /* ---------------- [2] 递归版 ---------------- */
    printf("[2] Recursive\n");
    g_out_n = 0; preorderTraversal(root);  printSeq("preorder");
    g_out_n = 0; inorderTraversal(root);   printSeq("inorder");
    g_out_n = 0; postorderTraversal(root); printSeq("postorder");
    printf("\n");

    /* ---------------- [3] 非递归版 ---------------- */
    printf("[3] Non-recursive (explicit stack / queue)\n");
    g_out_n = 0; preorderIter(root);  printSeq("preorder");
    g_out_n = 0; inorderIter(root);   printSeq("inorder");
    g_out_n = 0; postorderIter(root); printSeq("postorder");
    g_out_n = 0; levelOrder(root);    printSeq("level");
    printf("\n");

    /* ---------------- [4] 自检：两种实现必须完全一致 ---------------- */
    printf("[4] Self checks\n");
    g_out_n = 0; preorderIter(root);  expectSeq("iter preorder",  wantPre,  7);
    g_out_n = 0; inorderIter(root);   expectSeq("iter inorder",   wantIn,   7);
    g_out_n = 0; postorderIter(root); expectSeq("iter postorder", wantPost, 7);
    g_out_n = 0; levelOrder(root);    expectSeq("level order",    wantLvl,  7);

    g_out_n = 0; preorderTraversal(root);  expectSeq("rec  preorder",  wantPre,  7);
    g_out_n = 0; inorderTraversal(root);   expectSeq("rec  inorder",   wantIn,   7);
    g_out_n = 0; postorderTraversal(root); expectSeq("rec  postorder", wantPost, 7);
    printf("\n");

    /* ---------------- [5] 用遍历序列重建树 ---------------- */
    printf("[5] Rebuild the tree from its preorder + inorder sequences\n");
    Node *rebuilt = rebuild(wantPre, wantIn, 7);
    printf("  rebuilt shape:\n");
    printTree(rebuilt, 0, ' ');
    g_out_n = 0; inorderTraversal(rebuilt);
    expectSeq("rebuilt inorder", wantIn, 7);
    printf("\n");

    printf("  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [6] 释放 ---------------- */
    destroyTree(root);
    destroyTree(rebuilt);
    printf("[6] Both trees have been freed\n");

    return 0;
}
