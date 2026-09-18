/*
 * ============================================================================
 *  红黑树（Red-Black Tree, RBT）完整演示
 * ============================================================================
 *
 *  【一、红黑树是什么】
 *  它首先是一棵二叉搜索树（BST），只是给每个节点多刷了一种「颜色」：红或黑。
 *  颜色本身不存数据，唯一作用是约束树形，逼着树保持平衡。
 *
 *  【二、五条性质】（validateTree 会逐条校验）
 *        ① 每个节点非红即黑
 *        ② 根节点必须是黑色
 *        ③ 每个叶子（NULL 空指针）都视为黑色
 *        ④ 红节点的两个孩子必须是黑色（即：不允许出现连续的红色节点）
 *        ⑤ 从任一节点出发，到它所有后代 NULL 叶子的路径上，黑节点个数相同
 *           （这个个数叫「黑高」，black-height）
 *
 *  ④ + ⑤ 联手推出关键结论：  最长路径 ≤ 2 × 最短路径
 *  这就是「近似平衡」的来源：树高始终 O(log n)，
 *  不会像普通 BST 那样按有序数据插入就退化成一条链（见 bst.c 的 [4] 段演示）。
 *
 *  【三、和 AVL 树、B 树的关系】
 *        AVL 树 ：严格平衡（左右子树高差 ≤ 1），查询更快，但插入/删除旋转更多
 *        红黑树 ：近似平衡，旋转少（插入最多 2 次旋转），工程上更常用
 *                 —— C++ 的 std::map、Java 的 TreeMap、Linux 内核调度器都用它
 *        B 树   ：多路版本，一个节点放多个关键字，专治磁盘 I/O
 *
 *  【四、为什么节点里必须多一个 parent】
 *  插入修复要「顺着父链找爷爷和叔叔」，删除修复更要一路回溯，
 *  所以红黑树节点比 BST 多存一个 parent 指针。
 *  代价是：★ 每次旋转都必须同步维护 parent ★，这是本文件最容易写错的地方。
 *  可以和 avl_tree.c 对照着看 —— 那里没有 parent，旋转代码就短得多。
 *
 *  【五、插入修复的三种情况】（细节见 insertFixup 的注释）
 *  新节点一律染红（红色不影响黑高，最容易修复）。
 *  之后只看「父亲红不红」：
 *        情况 1：叔叔是红 → 父、叔变黑，爷爷变红，把问题上移到爷爷继续循环
 *        情况 2：叔叔是黑，且自己是「内侧」 → 先旋转自己，转成情况 3
 *        情况 3：叔叔是黑，且自己是「外侧」 → 父变黑、爷爷变红，旋转爷爷，收工
 *
 *  【六、文件结构】
 *     1. 类型定义      ← 保留原始代码，只补注释
 *     2. 小工具        colorOf / isRed / colorName
 *     3. 旋转          rotateLeft / rotateRight（必须维护 parent）
 *     4. 插入          insert / insertFixup
 *     5. 查找          search / minimum
 *     6. 删除          deleteNode / transplant / deleteFixup
 *     7. 校验          validateTree（五条性质 + BST 有序性 + parent 一致性）
 *     8. 遍历与统计    inorder / levelOrder / printTree / height / countNodes
 *     9. 释放          destroyTree
 *    10. main 演示     建树 / 查找 / 删除 / 200 次随机增删压力测试
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>    /* INT_MIN / INT_MAX，校验 BST 区间时当边界用 */

/* ============ 1. 类型定义（沿用原始定义，只补注释）============ */

/* 节点颜色。颜色不存数据，只用来约束树形 */
typedef enum {
    RED,
    BLACK
} Color;

typedef struct Node {
    int data;                  /* 关键字（本文件假设关键字不重复） */
    Color color;               /* 本节点的颜色 */
    struct Node *left;         /* 左孩子（值都比 data 小） */
    struct Node *right;        /* 右孩子（值都比 data 大） */
    struct Node *parent;       /* 父节点 —— 修复时要顺着它往上找叔叔 */
} Node;

/*
 * 造一个孤立的新节点。
 *
 * 注意新节点默认染成 RED，这不是随手定的：
 *   - 红色不影响任何路径的黑高（性质⑤），只可能违反「红不连红」（性质④）；
 *   - 若染成黑色，插入后几乎所有路径的黑高都会变，修复起来麻烦得多。
 * 所以「新节点入红、之后再修复」是红黑树插入的标准做法 ——
 * 原始代码里的那个 RED 正是这个意思。
 */
Node *createNode(int data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {              /* 分配失败要立刻退出，别拿着 NULL 继续写 */
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->color = RED;
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->parent = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------
 * C 编译器自上而下单遍扫描，函数必须先声明后使用。
 * 把原型集中写在这里，下面各函数的定义顺序就可以随意排。
 */
void  rotateLeft(Node **root, Node *x);
void  rotateRight(Node **root, Node *y);
bool  insert(Node **root, int data);
Node *search(Node *root, int data);
bool  deleteNode(Node **root, int data);
bool  validateTree(Node *root);
void  inorder(Node *root);
void  levelOrder(Node *root);
void  printTree(Node *node, int depth, char side);
int   height(Node *root);
int   countNodes(Node *root);
void  destroyTree(Node *root);

/* ============================ 2. 小工具 ============================ */

/*
 * 读一个节点的颜色。★ 关键约定：把 NULL 一律当作黑色 ★
 * 这不是偷懒，正是性质③「每个叶子（NULL）都是黑色」的直接落地。
 * 有了它，判断「叔叔是黑还是红」「兄弟是黑还是红」就不必再单独写 if (x == NULL)。
 */
static Color colorOf(Node *node) {
    return (node == NULL) ? BLACK : node->color;
}

static bool isRed(Node *node) {
    return colorOf(node) == RED;
}

/* 打印树形时用的颜色缩写 */
static const char *colorName(Color color) {
    return (color == RED) ? "R" : "B";
}

/* ============================ 3. 旋转 ============================ */

/*
 * 左旋：把 x 的右孩子 y 提上来顶替 x，x 自己降为 y 的左孩子。
 *
 *         x                    y
 *        / \                  / \
 *       a   y        -->     x   c
 *          / \              / \
 *         b   c            a   b
 *
 * 要改的挂接只有三处：y->left、x 的父节点的孩子指针、x 与 y 的父子关系。
 * ★ 但红黑树比 AVL 树多一个 parent 字段，所以这三处之外还要同步补 3~4 个
 *   parent 赋值。漏一个的典型症状是：本次旋转看起来没问题，
 *   但后续修复顺着错误的父链走偏，树悄悄坏掉（只有校验才能抓出来）。
 */
void rotateLeft(Node **root, Node *x) {
    Node *y = x->right;                 /* y 是 x 的右孩子，旋转后由它顶替 x */
    if (y == NULL) return;              /* 没有右孩子，转不动（正常调用不会发生） */

    x->right = y->left;                 /* ① y 的左子树 b 过继给 x 当右子树 */
    if (y->left != NULL) {
        y->left->parent = x;            /* b 换爸爸了，父指针要跟上 */
    }

    y->parent = x->parent;              /* ② y 接过 x 的位置（先接父指针） */
    if (x->parent == NULL) {
        *root = y;                      /* x 原来是根 → 现在 y 当根 */
    } else if (x == x->parent->left) {
        x->parent->left = y;            /* x 原来是左孩子 */
    } else {
        x->parent->right = y;           /* x 原来是右孩子 */
    }

    y->left = x;                        /* ③ x 降级，成为 y 的左孩子 */
    x->parent = y;
}

/*
 * 右旋：左旋的镜像操作，把 y 的左孩子 x 提上来顶替 y。
 *
 *           y                x
 *          / \              / \
 *         x   c    -->     a   y
 *        / \                  / \
 *       a   b                b   c
 */
void rotateRight(Node **root, Node *y) {
    Node *x = y->left;                  /* x 是 y 的左孩子，旋转后顶替 y */
    if (x == NULL) return;

    y->left = x->right;                 /* ① x 的右子树 b 过继给 y 当左子树 */
    if (x->right != NULL) {
        x->right->parent = y;
    }

    x->parent = y->parent;              /* ② x 接过 y 的位置 */
    if (y->parent == NULL) {
        *root = x;
    } else if (y == y->parent->left) {
        y->parent->left = x;
    } else {
        y->parent->right = x;
    }

    x->right = y;                       /* ③ y 降级，成为 x 的右孩子 */
    y->parent = x;
}

/* ============================ 4. 插入 ============================ */

static void insertFixup(Node **root, Node *z);

/*
 * 插入一个关键字。
 * 返回 true = 插入成功；false = 关键字已存在（本实现忽略重复值）。
 *
 * 所有红黑树操作都是这个套路，插入也不例外，分两步：
 *        第一步  BST 式查找 + 挂接 —— 只保证「有序」，不保证「平衡」
 *        第二步  insertFixup 修复   —— 只用旋转和变色，把被破坏的性质补回来
 * 好处是职责分离：找位置是 BST 的事，恢复红黑性质是红黑树的事。
 */
bool insert(Node **root, int data) {
    /* ---------- 第一步：按 BST 规则找到空位并挂上去 ---------- */
    Node *parent = NULL;
    Node *cur = *root;

    while (cur != NULL) {
        parent = cur;
        if (data < cur->data) {
            cur = cur->left;
        } else if (data > cur->data) {
            cur = cur->right;
        } else {
            return false;               /* 关键字已存在，不插入 */
        }
    }

    Node *z = createNode(data);         /* 新节点（createNode 里已染红） */
    z->parent = parent;

    if (parent == NULL) {
        *root = z;                      /* 原本是空树，新节点直接当根 */
    } else if (data < parent->data) {
        parent->left = z;
    } else {
        parent->right = z;
    }

    /* ---------- 第二步：修复被破坏的红黑性质 ---------- */
    insertFixup(root, z);
    return true;
}

/*
 * 插入修复 —— 循环处理「父亲是红色」这一种违规。
 *
 * 【为什么只需看父亲】
 * 新节点是红的：如果父亲是黑的，性质④没被破坏，黑高也没变 → 直接收工。
 * 只有「父亲也是红的」才出现连续红节点，需要修复。
 * 又因为根永远是黑的，所以「父亲是红」时祖父必然存在且必为黑。
 *
 * 【三种情况】（下面按「父是祖父的左孩子」写，另一种方向完全镜像）
 *
 *   情况 1：叔叔是红
 *         G(B)                      G(R)
 *        /    \                    /    \
 *      P(R)   U(R)      ==>      P(B)   U(B)
 *      /                         /
 *    Z(R)                      Z(R)
 *     做法：父、叔染黑，祖父染红，然后 z 跳到祖父继续循环。
 *     本质是「把这一层的黑往下压」，代价是祖父变红，可能跟曾祖父冲突。
 *
 *   情况 2：叔叔是黑，且 z 是「内侧」（即 LR 形：自己是父的右孩子，父是左孩子）
 *         G(B)                      G(B)
 *        /    \                    /    \
 *      P(R)   U(B)      ==>      Z(R)   U(B)
 *        \                       /
 *        Z(R)                  P(R)
 *     做法：先绕 P 左旋，把自己转到「外侧」，于是变成情况 3。
 *     这次旋转只改形状，不动颜色。
 *
 *   情况 3：叔叔是黑，且 z 是「外侧」（即 LL 形）
 *         G(B)                      P(B)
 *        /    \                    /    \
 *      P(R)   U(B)      ==>      Z(R)   G(R)
 *      /                                     \
 *    Z(R)                                   U(B)
 *     做法：父染黑、祖父染红，再绕祖父右旋，收工。
 *     旋转后这棵子树的根是黑的，性质④和⑤同时恢复。
 *
 * 【循环为什么一定终止】
 * 情况 1 每执行一次，z 至少上移两层，最多 O(log n) 次；
 * 情况 2 和情况 3 走完就直接结束。最后把根强制染黑（性质②）。
 */
static void insertFixup(Node **root, Node *z) {
    while (z->parent != NULL && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            /* ---------- 父亲是祖父的左孩子 ---------- */
            Node *uncle = z->parent->parent->right;

            if (isRed(uncle)) {
                /* 情况 1：叔叔红 → 父叔变黑、祖父变红，问题上移 */
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    /* 情况 2：内侧 → 绕父左旋，转成外侧 */
                    z = z->parent;
                    rotateLeft(root, z);
                }
                /* 情况 3：外侧 → 父变黑、祖父变红，绕祖父右旋 */
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotateRight(root, z->parent->parent);
            }
        } else {
            /* ---------- 父亲是祖父的右孩子（完全镜像） ---------- */
            Node *uncle = z->parent->parent->left;

            if (isRed(uncle)) {
                /* 情况 1（镜像） */
                z->parent->color = BLACK;
                uncle->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    /* 情况 2（镜像，RL 形）→ 绕父右旋 */
                    z = z->parent;
                    rotateRight(root, z);
                }
                /* 情况 3（镜像）→ 绕祖父左旋 */
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rotateLeft(root, z->parent->parent);
            }
        }
    }

    (*root)->color = BLACK;             /* 性质②：根永远是黑的 */
}

/* ============================ 5. 查找 ============================ */

/*
 * 红黑树首先是一棵 BST，所以查找逻辑和 bst.c 里一模一样。
 * 区别在于「树是平衡的」，所以最坏情况有保证：
 *        树高 ≤ 2·log2(n + 1)  →  查找 O(log n)
 * 而普通 BST 最坏是 O(n)（有序插入退化成链时）。
 */
Node *search(Node *root, int data) {
    while (root != NULL) {
        if (data == root->data) return root;                    /* 命中 */
        root = (data < root->data) ? root->left : root->right;  /* 小走左，大走右 */
    }
    return NULL;                                                /* 走到空位 = 不存在 */
}

/* 找以 node 为根的子树里的最小值（一路向左走到底）。
 * 删除「有两个孩子」的节点时，用它来找中序后继。
 */
static Node *minimum(Node *node) {
    if (node == NULL) return NULL;
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

/* ============================ 6. 删除 ============================ */

static void deleteFixup(Node **root, Node *x, Node *xParent);

/*
 * 用 v 顶替 u 的位置（只改挂接，不碰颜色，也不释放内存）。
 * 这是删除操作的最小积木，把「摘掉一个节点」的三种情形统一成一句调用。
 */
static void transplant(Node **root, Node *u, Node *v) {
    if (u->parent == NULL) {
        *root = v;                      /* u 是根 → v 直接当根 */
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;          /* v 的父指针要跟着改 */
    }
}

/*
 * 删除一个关键字。返回 true = 找到并删掉了，false = 不存在（什么都不做）。
 *
 * 和 bst.c 的 deleteNode 相比，这里多了一层「颜色账」：
 *   - 摘掉红节点 → 黑高完全没变，不需要修复；
 *   - 摘掉黑节点 → 那条路径少了一个黑，性质⑤被破坏，必须修复。
 *
 * 另一个实现选择：bst.c 用的是「搬值 + 递归删后继」，
 * 这里用「搬节点（transplant）」。红黑树更适合搬节点，因为每个节点
 * 还带着自己的颜色，若把值搬走、颜色留在原地，颜色账会更容易算错。
 *
 * 三种情形（和普通 BST 完全对应）：
 *        情形 A：没有左孩子 → 用右孩子顶替（右孩子可能是 NULL）
 *        情形 B：没有右孩子 → 用左孩子顶替
 *        情形 C：两个孩子都在 → 找中序后继 y（右子树最小值）来顶替自己
 */
bool deleteNode(Node **root, int data) {
    Node *z = search(*root, data);
    if (z == NULL) return false;        /* 树上没这个键，直接返回 */

    Node *y = z;                        /* y = 真正从树上被摘走的那个节点 */
    Color removedColor = y->color;      /* 被摘走节点的颜色，决定要不要修复 */
    Node *x = NULL;                     /* x = 顶替 y 位置的节点（可能是 NULL） */
    Node *xParent = NULL;               /* x 的父节点。x 可能是 NULL，必须单独记 */

    if (z->left == NULL) {
        /* 情形 A：没有左孩子 */
        x = z->right;
        xParent = z->parent;
        transplant(root, z, z->right);
    } else if (z->right == NULL) {
        /* 情形 B：没有右孩子 */
        x = z->left;
        xParent = z->parent;
        transplant(root, z, z->left);
    } else {
        /* 情形 C：两个孩子都在 —— 用右子树最小值（中序后继）顶替自己 */
        y = minimum(z->right);
        removedColor = y->color;
        x = y->right;

        if (y->parent == z) {
            /* 后继正好是自己的右孩子，直接接上 */
            xParent = y;
            if (x != NULL) x->parent = y;
        } else {
            /* 后继在更深处：先把它从原位置摘走，再让它顶替 z */
            xParent = y->parent;
            transplant(root, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        transplant(root, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;            /* 继承 z 的颜色 → 这棵子树的颜色账没变 */
    }

    free(z);                            /* z 的孩子都已被接管，现在可以安全释放 */

    /* 只有「被摘走的是黑节点」才需要修复 */
    if (removedColor == BLACK) {
        deleteFixup(root, x, xParent);
    }
    return true;
}

/*
 * 删除修复 —— 整个文件最难的一段。
 *
 * 【问题从哪来】
 * 摘掉一个黑节点后，经过它的路径黑节点数少了 1（性质⑤被破坏）。
 * 我们把这个「缺一个黑」的负担记在顶替上来的 x 上，称 x 为「双黑」。
 * 修复的目标：把这个多出来的黑「就地消化」或「一路往上推」，直到被吸收。
 *
 * 【四种情况】（按 x 是左孩子写，另一种方向完全镜像）
 *
 *   情况 1：兄弟 w 是红
 *        → w 染黑、父亲染红，绕父亲左旋。目的只是把兄弟换成黑的，
 *          从而进入下面三种情况之一。（不改黑高，只改形状）
 *
 *   情况 2：兄弟 w 是黑，且 w 的两个孩子都是黑
 *        → w 染红，把「双黑」推给父亲：x = 父亲，继续循环。
 *          本质是「整棵子树少了一个黑」，问题往上冒。
 *
 *   情况 3：兄弟 w 是黑，w 的右孩子是黑（也就是左孩子红）
 *        → w 的左孩子染黑、w 染红，绕 w 右旋，转成情况 4。
 *
 *   情况 4：兄弟 w 是黑，且 w 的右孩子是红      ← 终结情况
 *        → w 染成父亲的颜色，父亲和 w 的右孩子染黑，绕父亲左旋，
 *          然后把 x 设为根，循环结束。
 *
 * 【为什么需要 xParent 这个额外参数】
 * 本实现没有用哨兵节点，NULL 直接代表黑叶子（性质③）。
 * 于是 x 可能是 NULL，此时没法从 x 读 parent，也就无法往上回溯。
 * 所以调用方必须把 x 的父节点一起传进来 —— 这就是本函数有三个参数的原因。
 */
static void deleteFixup(Node **root, Node *x, Node *xParent) {
    while (x != *root && colorOf(x) == BLACK) {
        /* w 是 x 的兄弟。理论上 w 不可能是 NULL（否则黑高早已不等），
         * 这里的判空只是防御性编程，避免真出问题时直接崩溃。 */
        if (x == xParent->left) {
            /* ---------- x 是左孩子 ---------- */
            Node *w = xParent->right;
            if (w == NULL) { x = xParent; xParent = x->parent; continue; }

            if (colorOf(w) == RED) {
                /* 情况 1 */
                w->color = BLACK;
                xParent->color = RED;
                rotateLeft(root, xParent);
                w = xParent->right;
            }

            if (colorOf(w->left) == BLACK && colorOf(w->right) == BLACK) {
                /* 情况 2：把双黑推给父亲 */
                w->color = RED;
                x = xParent;
                xParent = x->parent;
            } else {
                if (colorOf(w->right) == BLACK) {
                    /* 情况 3：把内侧的红挪到外侧 */
                    w->left->color = BLACK;
                    w->color = RED;
                    rotateRight(root, w);
                    w = xParent->right;
                }
                /* 情况 4：终结，一次旋转收拾干净 */
                w->color = xParent->color;
                xParent->color = BLACK;
                w->right->color = BLACK;
                rotateLeft(root, xParent);
                x = *root;
                xParent = NULL;
            }
        } else {
            /* ---------- x 是右孩子（完全镜像） ---------- */
            Node *w = xParent->left;
            if (w == NULL) { x = xParent; xParent = x->parent; continue; }

            if (colorOf(w) == RED) {
                /* 情况 1（镜像） */
                w->color = BLACK;
                xParent->color = RED;
                rotateRight(root, xParent);
                w = xParent->left;
            }

            if (colorOf(w->left) == BLACK && colorOf(w->right) == BLACK) {
                /* 情况 2（镜像） */
                w->color = RED;
                x = xParent;
                xParent = x->parent;
            } else {
                if (colorOf(w->left) == BLACK) {
                    /* 情况 3（镜像） */
                    w->right->color = BLACK;
                    w->color = RED;
                    rotateLeft(root, w);
                    w = xParent->left;
                }
                /* 情况 4（镜像） */
                w->color = xParent->color;
                xParent->color = BLACK;
                w->left->color = BLACK;
                rotateRight(root, xParent);
                x = *root;
                xParent = NULL;
            }
        }
    }

    /* 循环结束后，x 身上多余的黑色就地消化（x 为 NULL 时无需处理） */
    if (x != NULL) x->color = BLACK;
}

/* ============================ 7. 校验 ============================ */

/*
 * 递归统计黑高，顺手检查「红不连红」。
 * 返回值 = 以 node 为根的子树黑高（NULL 记 1，即把空叶子本身算作一个黑）。
 * 一旦发现违反性质④或⑤，就把 *ok 置为 false，但继续走完。
 */
static int blackHeight(Node *node, bool *ok) {
    if (node == NULL) return 1;         /* 性质③：NULL 叶子算一个黑 */

    if (isRed(node) && (isRed(node->left) || isRed(node->right))) {
        *ok = false;                    /* 性质④被破坏：红节点有红孩子 */
    }

    int leftBH  = blackHeight(node->left,  ok);
    int rightBH = blackHeight(node->right, ok);
    if (leftBH != rightBH) {
        *ok = false;                    /* 性质⑤被破坏：左右黑高不相等 */
    }

    return leftBH + (node->color == BLACK ? 1 : 0);
}

/* BST 有序性：用「区间夹住整棵子树」的办法，和 bst.c 的 isBST 同一个思路 */
static bool inRange(Node *node, int lower, int upper) {
    if (node == NULL) return true;
    if (node->data <= lower || node->data >= upper) return false;
    return inRange(node->left,  lower, node->data) &&
           inRange(node->right, node->data, upper);
}

/*
 * parent 指针一致性 —— 红黑树独有的自检项。
 * 旋转或删除时若漏改 parent，BST 有序性和黑高可能都还正常，
 * 但下一次修复会顺着错误的父链走偏，于是错误被推迟到很远的地方才爆发。
 * 所以这里单独查一遍「每个节点的 parent 是否真的是它爸爸」。
 */
static bool parentsOK(Node *node, Node *expected) {
    if (node == NULL) return true;
    if (node->parent != expected) return false;
    return parentsOK(node->left, node) && parentsOK(node->right, node);
}

/*
 * 一次性校验：五条性质 + BST 有序性 + parent 一致性。
 * 教学代码里加这个函数非常划算：压力测试中每步调用一次，
 * 任何一处旋转/变色写错都会被立刻抓住，而不是等到最后才莫名崩溃。
 */
bool validateTree(Node *root) {
    if (root == NULL) return true;                     /* 空树合法 */

    if (root->color != BLACK) return false;            /* 性质②：根必须是黑 */
    if (root->parent != NULL) return false;            /* 根的 parent 必须是 NULL */

    bool ok = true;
    blackHeight(root, &ok);                            /* 性质③④⑤ */
    if (!ok) return false;

    return inRange(root, INT_MIN, INT_MAX) &&          /* BST 有序性 */
           parentsOK(root, NULL);                      /* parent 一致性 */
}

/* ======================= 8. 遍历、打印与统计 ======================= */

/*
 * 中序遍历：对 BST 来说输出必然升序。
 * 红黑树是 BST，所以这个性质同样成立，是校验有序性最直观的手段。
 * 三种深度优先遍历的差别，只在于 printf 放在哪一行（详见 bst.c 的说明）。
 */
void inorder(Node *root) {
    if (root == NULL) return;
    inorder(root->left);
    printf("%d ", root->data);
    inorder(root->right);
}

/*
 * 层序遍历（广度优先）：一层一层、从左到右打印，必须借助队列。
 * 这里用固定数组当队列，够演示用（不做动态扩容）。
 */
void levelOrder(Node *root) {
    if (root == NULL) return;

    Node *queue[256];
    int front = 0;
    int rear  = 0;

    queue[rear++] = root;                          /* 根入队 */

    while (front < rear) {
        Node *cur = queue[front++];                /* 出队一个 */
        printf("%d(%s) ", cur->data, colorName(cur->color));

        if (cur->left  != NULL) queue[rear++] = cur->left;
        if (cur->right != NULL) queue[rear++] = cur->right;
    }
}

/* 树高：空树 0，只有根节点为 1。
 * 红黑树的树高有保证：≤ 2·log2(n+1)。 */
int height(Node *root) {
    if (root == NULL) return 0;
    int lh = height(root->left);
    int rh = height(root->right);
    return (lh > rh ? lh : rh) + 1;
}

/* 节点总数 */
int countNodes(Node *root) {
    if (root == NULL) return 0;
    return 1 + countNodes(root->left) + countNodes(root->right);
}

/*
 * 打印整棵树的形状（带颜色标记）。
 * 缩进表示层级，L / R 表示是父节点的左孩子还是右孩子，(R)/(B) 是颜色：
 *
 *     30 (B)
 *         L 20 (R)
 *         R 40 (B)
 *
 * 全 ASCII 输出，不会乱码，也不用画连接线。
 */
void printTree(Node *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");

    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");

    printf("%d (%s)\n", node->data, colorName(node->color));

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* ============================ 9. 释放 ============================ */

/*
 * 递归释放整棵树。
 * 顺序必须是「先释放两个孩子，最后释放自己」——
 * 反过来的话，free 之后你就找不到孩子了，内存泄漏。
 */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* ============================ 10. main 演示 ============================ */

/* 小型伪随机数发生器：固定种子 → 每次运行结果完全一致，便于对比和排查 */
static unsigned int g_seed = 20260918u;

static unsigned int nextRand(void) {
    g_seed = g_seed * 1103515245u + 12345u;   /* 线性同余，无符号溢出即取模 */
    return (g_seed >> 16) & 0x7FFFu;          /* 取高位，低位随机性差 */
}

/* 洗牌（Fisher-Yates），保证每个排列等概率 */
static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(nextRand() % (unsigned int)(i + 1));
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* 把中序遍历结果收集到数组里，返回写入个数 */
static int collectInorder(Node *node, int *out, int idx) {
    if (node == NULL) return idx;
    idx = collectInorder(node->left, out, idx);
    out[idx++] = node->data;
    return collectInorder(node->right, out, idx);
}

/* 独立于 validateTree 的另一种有序性检查：中序结果是否严格升序 */
static bool inorderSorted(Node *root, int count) {
    int buf[512];                       /* 压力测试最多 200 个节点，够用 */
    if (count > (int)(sizeof(buf) / sizeof(buf[0]))) return false;

    if (collectInorder(root, buf, 0) != count) return false;
    for (int i = 1; i < count; i++) {
        if (buf[i - 1] >= buf[i]) return false;
    }
    return true;
}

int main(void) {
    printf("==========================================================\n");
    printf("  Red-Black Tree (RBT) demo\n");
    printf("==========================================================\n");
    printf("  Legend: (R) = red node, (B) = black node\n\n");

    /* ---------------- [1] 插入：有序数据也不会退化 ---------------- */
    Node *root = NULL;
    int keys[] = {10, 20, 30, 40, 50, 60};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    printf("[1] Insert ascending keys: ");
    for (int i = 0; i < n; i++) printf("%d ", keys[i]);
    printf("\n");
    printf("    A plain BST would degenerate into a chain of height %d (see bst.c),\n", n);
    printf("    but the red-black tree re-balances itself after every insert.\n\n");

    for (int i = 0; i < n; i++) {
        insert(&root, keys[i]);
        printf("---- after inserting %d ----\n", keys[i]);
        printTree(root, 0, ' ');
        printf("nodes: %d    height: %d    valid RBT: %s\n\n",
               countNodes(root), height(root), validateTree(root) ? "YES" : "NO");
    }

    printf("    Inorder  : "); inorder(root);    printf("\n");
    printf("    Level    : "); levelOrder(root); printf("\n\n");

    /* ---------------- [2] 查找 ---------------- */
    printf("[2] Search\n");
    int query[] = {30, 65, 10};
    for (int i = 0; i < 3; i++) {
        Node *found = search(root, query[i]);
        if (found != NULL) {
            printf("  %3d -> found, color = %s\n", query[i], colorName(found->color));
        } else {
            printf("  %3d -> not found\n", query[i]);
        }
    }
    printf("\n");

    /* ---------------- [3] 删除：把几种经典情形逐个走一遍 ---------------- */
    printf("[3] Delete demo\n");
    printf("    Each step first reports what kind of node will be removed,\n");
    printf("    then deletes it and re-validates the whole tree.\n\n");

    int delKeys[] = {60, 40, 50, 20, 10, 30, 999};
    int delCount = (int)(sizeof(delKeys) / sizeof(delKeys[0]));

    for (int i = 0; i < delCount; i++) {
        int key = delKeys[i];

        /* 先查这个键在不在，并数一数它有几个孩子。
         * 这样打印出的说明永远与真实结构一致，不靠人肉推断。 */
        Node *z = search(root, key);
        if (z == NULL) {
            printf("---- delete %d : key not found ----\n", key);
        } else {
            int kids = (z->left != NULL) + (z->right != NULL);
            const char *kind = (kids == 0) ? "leaf"
                             : (kids == 1) ? "node with one child"
                                           : "node with two children";
            printf("---- delete %d : %s %s ----\n",
                   key, (z->color == RED) ? "red" : "black", kind);
        }

        deleteNode(&root, key);

        if (root == NULL) {
            printf("(empty tree)\n");
        } else {
            printTree(root, 0, ' ');
        }
        printf("nodes: %d    height: %d    valid RBT: %s\n",
               countNodes(root), height(root), validateTree(root) ? "YES" : "NO");
        printf("inorder: "); inorder(root); printf("\n\n");
    }

    destroyTree(root);
    root = NULL;

    /* ---------------- [4] 随机压力测试 ---------------- */
    printf("[4] Stress test: 200 random inserts + 200 random deletes\n");
    printf("    After every single operation we verify all 5 RBT properties,\n");
    printf("    BST ordering and parent pointers.\n\n");

    enum { STRESS_N = 200 };
    int data[STRESS_N];
    for (int i = 0; i < STRESS_N; i++) data[i] = i + 1;

    Node *big = NULL;
    int steps = 0, pass = 0, fail = 0;
    bool sortedOK = true;

    /* 第一段：把 1..200 打乱顺序插进去，每插一个校验一次 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        insert(&big, data[i]);
        steps++;
        if (validateTree(big)) pass++; else fail++;
        if (!inorderSorted(big, countNodes(big))) sortedOK = false;
    }
    printf("    after %d inserts : nodes = %d, height = %d\n",
           STRESS_N, countNodes(big), height(big));
    printf("    (theoretical bound 2*log2(n+1) ~= 15; a plain BST could reach %d)\n\n",
           STRESS_N);

    /* 第二段：再打乱一次顺序，把所有键逐个删光 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        if (!deleteNode(&big, data[i])) {
            printf("    ERROR: key %d should exist but was not found\n", data[i]);
            fail++;
        }
        steps++;
        if (validateTree(big)) pass++; else fail++;
        if (!inorderSorted(big, countNodes(big))) sortedOK = false;
    }

    /* 再删几个不存在的键，确认不会误删、也不会破坏结构 */
    for (int k = 1000; k < 1005; k++) {
        if (deleteNode(&big, k)) {
            printf("    ERROR: missing key %d was reported as deleted\n", k);
            fail++;
        }
        if (validateTree(big)) pass++; else fail++;
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
