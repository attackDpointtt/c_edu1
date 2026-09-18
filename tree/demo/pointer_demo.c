/*
 * ============================================================================
 *  指针传参演示 —— 从 int 到 Node *，看懂 Node **root
 * ============================================================================
 *
 *  【为什么要有这个 demo】
 *  red-black_tree.c 里到处是 Node **root，读起来既像「解引用」又像「引用」，
 *  很容易绕晕。这里用最小的例子把规律钉死：
 *
 *      ★ 想改什么类型，形参就在它后面再加一个 * ★
 *
 *        想改 int            →  形参 int *      调用 f(&a)      体内 *p = 100
 *        想改 int *（指针）  →  形参 int **     调用 f(&p)      体内 *q = NULL
 *        想改 Node *（根）   →  形参 Node **    调用 f(&root)   体内 *root = z
 *
 *  【两个必须记住的事实】
 *  1. C 是值传递：形参永远是实参的一份副本。
 *  2. 副本能改到外面的前提，是副本里装着「外面那个变量」的地址。
 *     一级指针：副本里装着「int 的地址」      → 能改 int；
 *     二级指针：副本里装着「指针变量的地址」  → 能改指针。
 *
 *  【编译】
 *      gcc -Wall -Wextra -std=c11 pointer_demo.c -o pointer_demo.exe
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

/* 迷你节点：只为演示「改根指针」，字段和 red-black_tree.c 的 Node 同构 */
typedef struct Node {
    int data;
    struct Node *left;
    struct Node *right;
} Node;

/* ---------------------------- 四个 int 级例子 ---------------------------- */

/*
 * 情况 1：只想改个 int，却只传了值。
 * n 是实参 a 的副本，改 n 跟 a 毫无关系。
 */
static void try1(int n) {
    n = 100;
    printf("        (inside try1: n = %d, but n is only a copy)\n", n);
}

/*
 * 情况 2：想改 int，传的是「a 的地址」。
 * p 里装着 a 的地址，*p 就是 a 本人，所以改得动。
 */
static void try2(int *p) {
    *p = 100;
}

/*
 * 情况 3：想改「指针 p 自己」，却只传了 p 的值。
 * q 是 p 的副本：*q 能读到 p 指向的东西（说明它没指错），
 * 但 q = NULL 只是把副本清空，外面的 p 纹丝不动。
 */
static void try3(int *q) {
    q = NULL;
    printf("        (inside try3: q %s NULL, but q is only a copy)\n",
           q ? "!=" : "==");
}

/*
 * 情况 4：想改「指针 p 自己」，传的是「p 的地址」。
 * q 指向 p 这个变量，*q 就是 p 本人，所以 *q = NULL 真的把 p 置空了。
 * ★ 这就是 Node **root 的来历 ★
 */
static void try4(int **q) {
    *q = NULL;
}

/* ------------------------- 对应红黑树里的 Node ** ------------------------- */

/*
 * 迷你版「往空树里插第一个节点」。
 * 树还是空的时候 root 是 NULL，插入必须把「main 里的 root 变量」改成新节点，
 * 所以形参只能是 Node **，体内写 *root = z ——
 * 和 red-black_tree.c 里 insert() 的 `*root = z;` 完全同构。
 *
 * 若形参写成 Node *root，那么 `root = z;` 只改副本，
 * main 里的 root 依然是 NULL，树根本建不起来（而且编译器不会报错）。
 */
static void insertFirst(Node **root, int data) {
    Node *z = (Node *)malloc(sizeof(Node));
    if (z == NULL) {                    /* 分配失败立刻退出，别拿着 NULL 继续写 */
        printf("malloc failed\n");
        exit(EXIT_FAILURE);
    }
    z->data = data;
    z->left = NULL;
    z->right = NULL;

    *root = z;                          /* 解一层 = 改到调用方那个 root 变量 */
}

int main(void) {
    int a = 1, b = 1, c = 999;
    int *p = &c;

    printf("==========================================================\n");
    printf("  Pointer arguments demo: from int to Node *\n");
    printf("==========================================================\n\n");

    printf("Case 1: try1(int n)      -- change an int, no address passed\n");
    try1(a);
    printf("        a = %d              NOT changed (n is a copy)\n\n", a);

    printf("Case 2: try2(int *p)     -- change an int, address passed\n");
    try2(&b);
    printf("        b = %d            CHANGED (*p is b itself)\n\n", b);

    printf("Case 3: try3(int *q)     -- change a POINTER, only its value passed\n");
    printf("        before: p %s NULL, *p = %d\n", p ? "!=" : "==", *p);
    try3(p);
    printf("        after : p %s NULL, *p = %d\n", p ? "!=" : "==", *p);
    printf("        p NOT changed (q is a copy of p)\n\n");

    printf("Case 4: try4(int **q)    -- change a POINTER, its address passed\n");
    printf("        before: p %s NULL\n", p ? "!=" : "==");
    try4(&p);
    printf("        after : p %s NULL\n", p ? "!=" : "==");
    printf("        p CHANGED (*q is p itself)\n\n");

    printf("Case 5: insertFirst(Node **root, int data) -- the real thing\n");
    Node *root = NULL;
    printf("        before: root %s NULL\n", root ? "!=" : "==");
    insertFirst(&root, 7);
    printf("        after : root %s NULL, root->data = %d\n",
           root ? "!=" : "==", root->data);
    printf("        the root variable in main() was really updated\n\n");

    printf("Rule: to change a value of type T, the parameter must be T *\n");
    printf("        int    -> int *      (call f(&a))\n");
    printf("        int *  -> int **     (call f(&p))\n");
    printf("        Node * -> Node **    (call f(&root))\n");

    free(root);
    return 0;
}
