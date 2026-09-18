/*
 * ============================================================================
 *  二叉搜索树（Binary Search Tree, BST）完整演示
 * ============================================================================
 *
 *  【一、什么是二叉搜索树】
 *  一棵二叉树，并且对任意节点都满足：
 *
 *        左子树里所有值  <  该节点的值  <  右子树里所有值
 *
 *  这条性质让查找可以「每次砍掉一半」：
 *        比当前节点小 → 往左走
 *        比当前节点大 → 往右走
 *
 *  【二、和后面要学的 B 树什么关系】
 *
 *        BST  ： 1 个关键字 + 2 个孩子
 *        B 树 ： k 个关键字 + k+1 个孩子     ← BST 的「多路」推广
 *
 *  所以先把 BST 吃透，B 树就只是「一个节点里塞多个数」而已。
 *  本文件里的递归写法（insert / deleteNode / 遍历 / 校验），
 *  在 B 树的代码里会以几乎相同的形态再出现一次。
 *
 *  【三、BST 的致命问题（这就是 B 树诞生的动机）】
 *  如果按从小到大的顺序插入，BST 会退化成一条链：
 *
 *        10
 *          \
 *           20
 *             \
 *              30   ...
 *
 *  此时查找变成 O(n)，和链表一样慢，「搜索树」名存实亡。
 *  于是有了两条路：
 *    - AVL / 红黑树：靠「旋转」重新平衡，但每个节点仍然只放 1 个数
 *    - B 树        ：靠「多路 + 分裂」把树压矮，每个节点放多个数
 *
 *  本文件的最后一段演示 [4] 会亲眼看到退化。
 *
 *  【四、文件结构】
 *    1. 结构体与基础工具
 *    2. 插入        insert
 *    3. 查找        search / findMin / findMax
 *    4. 删除        deleteNode   ← 三种情况，本文件最难的部分
 *    5. 遍历        前序 / 中序 / 后序 / 层序
 *    6. 统计与校验  height / countNodes / isBST
 *    7. 打印与释放  printTree / destroyTree
 *    8. main 演示
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>    /* INT_MIN / INT_MAX，isBST 校验时当区间边界用 */

/* ======================== 1. 结构体与基础工具 ======================== */

typedef struct Node {
    int data;                  /* 这个节点存的值 */
    struct Node *left;         /* 左孩子（值都比 data 小） */
    struct Node *right;        /* 右孩子（值都比 data 大） */
} Node;

/*
 * 造一个孤立的新节点（左右孩子都是空的）。
 * 注意：malloc 出来的内存里是垃圾值，所以三个字段都必须显式赋值。
 */
Node *createNode(int data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data  = data;
    newNode->left  = NULL;
    newNode->right = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------
 * C 编译器自上而下单遍扫描，函数必须先声明后使用。
 * 把这些原型写在前面，下面函数的定义顺序就可以随便排。
 */
Node *insert(Node *root, int data);
Node *search(Node *root, int data);
Node *findMin(Node *root);
Node *findMax(Node *root);
Node *deleteNode(Node *root, int data);
void  preorder(Node *root);
void  inorder(Node *root);
void  postorder(Node *root);
void  levelOrder(Node *root);
int   height(Node *root);
int   countNodes(Node *root);
bool  isBST(Node *root, int lower, int upper);
void  printTree(Node *node, int depth, char side);
void  destroyTree(Node *root);

/* ============================ 2. 插入 ============================ */

/*
 * 插入一个值。
 *
 * 思路：一路递归向下比较，直到走进一片空位（NULL），在那里挂上新节点。
 *
 * 注意返回值：返回的是「插入之后的子树根」。
 * 父节点必须把自己的 left / right 重新接住这个返回值，所以写成
 *        root->left = insert(root->left, data);
 * 这和 B 树里 root->left = insert(root->left, ...) 是完全同一个套路。
 *
 * 本实现忽略重复值（BST 通常假设元素不重复）。
 */
Node *insert(Node *root, int data) {
    if (root == NULL) {
        return createNode(data);            /* 找到空位了，造节点交出去 */
    }

    if (data < root->data) {
        root->left = insert(root->left, data);      /* 小 → 去左边 */
    } else if (data > root->data) {
        root->right = insert(root->right, data);    /* 大 → 去右边 */
    }
    /* data == root->data 时什么都不做，直接返回（忽略重复） */

    return root;
}

/* ============================ 3. 查找 ============================ */

/*
 * 查找值为 data 的节点，找到返回它的地址，找不到返回 NULL。
 *
 * 这正是 BST 的优势：每比较一次就能排除掉一半子树。
 * 树平衡时是 O(log n)；树退化成链时是 O(n)。
 */
Node *search(Node *root, int data) {
    if (root == NULL) {
        return NULL;                        /* 走到空位，说明不存在 */
    }
    if (data == root->data) {
        return root;                        /* 命中 */
    }
    if (data < root->data) {
        return search(root->left, data);    /* 小 → 去左边找 */
    }
    return search(root->right, data);       /* 大 → 去右边找 */
}

/*
 * 找最小值：BST 里最小的值一定在「最左边」。
 * 所以一路往左走到底就行，不用递归。
 */
Node *findMin(Node *root) {
    if (root == NULL) return NULL;
    while (root->left != NULL) {
        root = root->left;
    }
    return root;
}

/*
 * 找最大值：一定在「最右边」。
 */
Node *findMax(Node *root) {
    if (root == NULL) return NULL;
    while (root->right != NULL) {
        root = root->right;
    }
    return root;
}

/* ============================ 4. 删除 ============================ */

/*
 * 删除节点 —— BST 里最麻烦的操作，按被删节点的孩子数量分三种情况：
 *
 *   情况 ①  要删的是叶子（没有孩子）
 *            → 直接摘掉，把 NULL 交还给父节点
 *
 *   情况 ②  要删的节点只有一个孩子
 *            → 让这个孩子「顶替」它的位置：把孩子的地址交还给父节点
 *
 *   情况 ③  要删的节点有两个孩子        ← 最麻烦
 *            → 不能直接摘，否则左右两棵子树没法接起来。
 *              做法是找一个「替身」：右子树里最小的那个（叫「中序后继」）。
 *              把替身的「值」复制到当前节点，
 *              然后递归去右子树里把那个替身删掉。
 *
 *              为什么这招一定成功？
 *              因为后继是右子树里最小的，它不可能有左孩子
 *              （有左孩子就说明它不是最小的了）。
 *              所以「递归删后继」一定落回情况①或②，不会无限递归。
 *
 * 情况①和情况②在代码里可以合并：只要「至少有一个孩子是空的」，
 * 就把另一个孩子交出去（可能是 NULL，也可能是一整棵子树）。
 */
Node *deleteNode(Node *root, int data) {
    if (root == NULL) {
        return NULL;                        /* 找遍了都没这个人，什么都不做 */
    }

    if (data < root->data) {
        root->left = deleteNode(root->left, data);      /* 去左边删 */
    } else if (data > root->data) {
        root->right = deleteNode(root->right, data);    /* 去右边删 */
    } else {
        /* ---------- 找到了，开始分情况 ---------- */

        /* 情况① + 情况②：至少有一个孩子是空的 */
        if (root->left == NULL) {
            Node *rightChild = root->right;  /* 先记下右孩子（可能是 NULL） */
            free(root);                      /* 再释放自己 */
            return rightChild;               /* 把右孩子交还给父节点 */
        }
        if (root->right == NULL) {
            Node *leftChild = root->left;
            free(root);
            return leftChild;
        }

        /* 情况③：两个孩子都在 */
        Node *succ = findMin(root->right);           /* 中序后继 = 右子树最小值 */
        root->data = succ->data;                     /* 只搬「值」，不搬节点 */
        root->right = deleteNode(root->right, succ->data);  /* 再去右边把替身删掉 */
    }

    return root;
}

/* ============================ 5. 遍历 ============================ */

/*
 * 遍历就是把所有节点「每个都访问一次」。
 * 三种深度优先遍历，差别只在「什么时候处理根节点」：
 *
 *        前序 preorder ：根 → 左 → 右
 *        中序 inorder  ：左 → 根 → 右     ★ BST 的中序输出一定是升序 ★
 *        后序 postorder：左 → 右 → 根
 *
 * 递归写法几乎一模一样，只是 printf 的位置换了一下。
 * 这一点很值得记住：把 printf 挪个位置，就是另一种遍历。
 */

void preorder(Node *root) {
    if (root == NULL) return;
    printf("%d ", root->data);     /* 先处理根 */
    preorder(root->left);
    preorder(root->right);
}

/*
 * 中序：BST 的「排序输出」。
 * 因为顺序是「左（全比根小）→ 根 → 右（全比根大）」，
 * 所以打印出来必然从小到大。
 * 判断一棵树是不是合法 BST，最直观的方法就是看中序是否升序。
 */
void inorder(Node *root) {
    if (root == NULL) return;
    inorder(root->left);
    printf("%d ", root->data);     /* 中间处理根 */
    inorder(root->right);
}

void postorder(Node *root) {
    if (root == NULL) return;
    postorder(root->left);
    postorder(root->right);
    printf("%d ", root->data);     /* 最后处理根 */
}

/*
 * 层序遍历（广度优先）：一层一层、从左到右打印。
 *
 * 它不能靠递归实现，必须借助一个「队列」（先进先出）：
 *    1. 根入队
 *    2. 循环：队头出队并打印，然后把它的左孩子、右孩子依次入队
 *
 * 这里用最简单的数组当队列，够演示用（不做动态扩容）。
 */
void levelOrder(Node *root) {
    if (root == NULL) return;

    Node *queue[128];              /* 简单队列：存的是节点地址 */
    int front = 0;                 /* 队头下标 */
    int rear  = 0;                 /* 队尾下标（下一个空位） */

    queue[rear++] = root;          /* 根入队 */

    while (front < rear) {
        Node *cur = queue[front++];          /* 出队一个 */
        printf("%d ", cur->data);

        if (cur->left  != NULL) queue[rear++] = cur->left;    /* 左孩子入队 */
        if (cur->right != NULL) queue[rear++] = cur->right;   /* 右孩子入队 */
    }
}

/* ======================= 6. 统计与校验 ======================= */

/*
 * 求树高：空树算 0，只有根节点算 1。
 * 等于「左子树高度」和「右子树高度」里较大的那个，再加 1（算自己）。
 */
int height(Node *root) {
    if (root == NULL) return 0;
    int lh = height(root->left);
    int rh = height(root->right);
    return (lh > rh ? lh : rh) + 1;
}

/* 统计节点总数 */
int countNodes(Node *root) {
    if (root == NULL) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

/*
 * 校验：这棵树到底是不是一棵合法的 BST。
 *
 * ⚠️ 不能只检查「父节点 vs 直接孩子」，那样查不出下面这种错误：
 *
 *              10
 *             /  \
 *            5    15
 *                /
 *               3        ← 3 比 10 小，却藏在右子树里，非法！
 *
 * 正确做法：从上往下传一个「允许区间 (lower, upper)」，
 * 每个节点都必须严格落在自己的区间内；
 * 往左走时上界收紧成当前值，往右走时下界收紧成当前值。
 *
 * 这招和 B 树校验（validateSubtree）用的是同一个思路：
 * 「用区间夹住整棵子树」，而不是只看父子两代。
 */
bool isBST(Node *root, int lower, int upper) {
    if (root == NULL) return true;                                 /* 空子树合法 */
    if (root->data <= lower || root->data >= upper) return false;   /* 越界了 */
    return isBST(root->left,  lower, root->data) &&                /* 左边区间收紧上界 */
           isBST(root->right, root->data, upper);                  /* 右边区间收紧下界 */
}

/* ======================= 7. 打印与释放 ======================= */

/*
 * 打印整棵树的形状。
 *
 * 缩进表示层级，L / R 表示这个节点是父节点的左孩子还是右孩子：
 *
 *     50
 *         L 30
 *             L 20
 *             R 40
 *                 L 35
 *                 R 45
 *         R 70
 *             L 60
 *             R 80
 *
 * 这样不用画连接线就能一眼看清结构，而且输出全是 ASCII，不会乱码。
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

/*
 * 递归释放整棵树。
 * 顺序必须是「先释放两个孩子，最后释放自己」——
 * 反过来的话，free(root) 之后你就找不到孩子了，内存泄漏。
 */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* ============================ 8. main 演示 ============================ */

/* 打印当前树的形状、统计信息和各种遍历结果 */
void showState(Node *root, const char *title) {
    printf("-------- %s --------\n", title);

    if (root == NULL) {
        printf("(empty tree)\n\n");
        return;
    }

    printf("Tree shape (L = left child, R = right child):\n");
    printTree(root, 0, ' ');

    printf("Total nodes: %d    Height: %d\n", countNodes(root), height(root));
    printf("Min: %d    Max: %d\n", findMin(root)->data, findMax(root)->data);
    printf("Preorder : "); preorder(root);   printf("\n");
    printf("Inorder  : "); inorder(root);    printf("\n");
    printf("Postorder: "); postorder(root);  printf("\n");
    printf("Level    : "); levelOrder(root); printf("\n");
    printf("Is valid BST: %s\n\n", isBST(root, INT_MIN, INT_MAX) ? "YES" : "NO");
}

/* ==================== 自检小工具（与其它演示文件统一） ==================== */

static int g_pass = 0;
static int g_fail = 0;

/* 比对实测值与预期值，打印一行结论并累计通过/失败数 */
static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-34s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-34s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

/* 把中序结果收进数组：原来的 inorder() 只负责打印，自检需要能拿到序列 */
static int collectInorder(Node *node, int *out, int idx) {
    if (node == NULL) return idx;
    idx = collectInorder(node->left, out, idx);
    out[idx++] = node->data;
    return collectInorder(node->right, out, idx);
}

/* 检查中序序列是否与预期完全一致（BST 的中序必须升序） */
static void expectSorted(const char *what, Node *root, const int *want, int n) {
    int got[64];
    int gotN = collectInorder(root, got, 0);

    bool same = (gotN == n);
    for (int i = 0; same && i < n; i++) {
        if (got[i] != want[i]) same = false;
    }

    printf("  [%s] %-34s:", same ? "ok  " : "FAIL", what);
    for (int i = 0; i < gotN; i++) printf(" %d", got[i]);
    if (!same) {
        printf("  (expected:");
        for (int i = 0; i < n; i++) printf(" %d", want[i]);
        printf(" )");
    }
    printf("\n");

    if (same) g_pass++; else g_fail++;
}

int main(void) {
    Node *root = NULL;

    printf("=========================================================\n");
    printf("  Binary Search Tree (BST) demo\n");
    printf("=========================================================\n\n");

    /* ---------------- [1] 建树 ---------------- */
    int keys[] = {50, 30, 70, 20, 40, 60, 80, 35, 45};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    printf("[1] Build the tree by inserting: ");
    for (int i = 0; i < n; i++) printf("%d ", keys[i]);
    printf("\n\n");

    for (int i = 0; i < n; i++) {
        root = insert(root, keys[i]);
    }

    showState(root, "after building");

    /* ---------------- [2] 查找 ---------------- */
    printf("[2] Search\n");
    int query[] = {35, 100, 50};
    for (int i = 0; i < 3; i++) {
        Node *found = search(root, query[i]);
        if (found != NULL) {
            printf("  %3d -> found (node address %p)\n", query[i], (void *)found);
        } else {
            printf("  %3d -> not found\n", query[i]);
        }
    }
    printf("\n");

    /* ---------------- [3] 删除：三种情况 ---------------- */
    printf("[3] Delete demo -- three cases\n");
    printf("    case 1: delete a leaf\n");
    printf("    case 2: delete a node with exactly one child\n");
    printf("    case 3: delete a node with two children\n\n");

    /* 情况①：20 是叶子，直接摘掉 */
    root = deleteNode(root, 20);
    showState(root, "delete 20 (a leaf)");

    /* 情况②：20 没了之后，30 只剩右孩子 40，让 40 顶上来 */
    root = deleteNode(root, 30);
    showState(root, "delete 30 (one child)");

    /* 情况③：50 是根，左右孩子都在 → 用右子树最小值 60 顶替 */
    root = deleteNode(root, 50);
    showState(root, "delete 50 (two children)");

    /* 删除一个不存在的值 */
    root = deleteNode(root, 999);
    printf("(deleting a value that does not exist changes nothing)\n\n");

    destroyTree(root);
    root = NULL;

    /* ---------------- [4] 退化演示：为什么要学 B 树 ---------------- */
    printf("[4] Why BST alone is not enough\n\n");
    printf("    Insert already-sorted data: 10 20 30 40 50 60\n\n");

    Node *skewed = NULL;
    int sortedKeys[] = {10, 20, 30, 40, 50, 60};
    for (int i = 0; i < 6; i++) {
        skewed = insert(skewed, sortedKeys[i]);
    }

    showState(skewed, "after inserting sorted data");

    printf("    6 nodes but height is %d -- the tree became a linked list.\n",
           height(skewed));
    printf("    Search cost degrades from O(log n) to O(n).\n");
    printf("    This is exactly the problem that B-trees solve.\n\n");

    /* ---------------- [5] 自检 ---------------- */
    printf("[5] Self checks\n");

    /* 前面的树在 [3] 段已经删掉并释放了，这里重新建一棵同样的树来做断言 */
    Node *t = NULL;
    for (int i = 0; i < n; i++) {
        t = insert(t, keys[i]);
    }

    static const int wantInorder[] = {20, 30, 35, 40, 45, 50, 60, 70, 80};

    expectInt("node count",     countNodes(t), 9);
    expectInt("height",         height(t),     4);
    expectInt("min value",      findMin(t)->data, 20);
    expectInt("max value",      findMax(t)->data, 80);
    expectInt("is a valid BST", isBST(t, INT_MIN, INT_MAX) ? 1 : 0, 1);
    expectSorted("inorder is ascending", t, wantInorder, 9);

    /* 把删除的三种情况再走一遍，用节点数验证每次都只少了 1 个 */
    t = deleteNode(t, 20);
    expectInt("after deleting a leaf (20)",           countNodes(t), 8);
    t = deleteNode(t, 30);
    expectInt("after deleting a one-child node (30)",  countNodes(t), 7);
    t = deleteNode(t, 50);
    expectInt("after deleting a two-child node (50)",  countNodes(t), 6);
    t = deleteNode(t, 999);
    expectInt("deleting a missing key (999)",          countNodes(t), 6);
    expectInt("still a valid BST after deletions",
              isBST(t, INT_MIN, INT_MAX) ? 1 : 0, 1);

    destroyTree(t);

    /* [4] 段那棵退化树：6 个节点、高度 6 —— 就是一条链 */
    expectInt("sorted input degenerates: nodes", countNodes(skewed), 6);
    expectInt("sorted input degenerates: height", height(skewed),  6);

    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [6] 释放 ---------------- */
    destroyTree(skewed);

    printf("[6] All trees have been freed\n");

    return 0;
}
