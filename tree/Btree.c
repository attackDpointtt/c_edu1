/*
 * ============================================================================
 *  B 树（B-Tree）完整演示 —— M 阶 B 树，本文件取 M = 4
 * ============================================================================
 *
 *  【一、什么才是「真正的」B 树】
 *  一棵 M 阶 B 树必须同时满足下面 5 条性质，缺一条就不算 B 树：
 *
 *    1. 每个节点最多 M 个子节点（因此最多 M-1 个关键字）
 *    2. 除根以外，每个节点至少有 ceil(M/2) 个子节点
 *       （因此至少 ceil(M/2)-1 个关键字）
 *    3. 根节点至少有 1 个关键字（空树除外）
 *    4. 节点内关键字严格升序，并且满足"分隔关系"：
 *           keys[0] < keys[1] < ... < keys[n-1]
 *           children[i] 子树里所有关键字都落在 (keys[i-1], keys[i]) 之间
 *    5. 所有叶子节点都在同一层（绝对平衡）
 *
 *  第 5 条是 B 树最核心的价值：无论插入、删除怎么折腾，树高始终一致，
 *  于是查找 / 插入 / 删除的复杂度稳定在 O(log n)，不会退化成长链表。
 *
 *  本文件 M = 4，也就是常说的「2-3-4 树」：
 *  每个节点 1~3 个关键字，2~4 个孩子。
 *
 *  【二、文件结构】
 *    1. 结构体与基础工具
 *    2. 查找            search
 *    3. 插入            insert / insertNonFull / splitChild
 *    4. 删除            deleteKey / deleteKeyRec / fillChild
 *                       borrowFromLeft / borrowFromRight / mergeChildren
 *    5. B 树性质校验    validateTree   ← 用来证明「它真的是 B 树」
 *    6. 遍历、打印、销毁
 *    7. main 演示
 *
 *  【三、相比原始版本修掉的问题】
 *    (1) 函数「先用后定义」：merge 和 deleteKey 在被调用时还没声明，
 *        gcc 会报 implicit declaration 与 conflicting types，编译直接失败。
 *        → 修法：文件开头统一写前向声明。
 *    (2) insert 遇到空树时只造了一个空节点，key 被丢掉，
 *        导致第一个插入的数据凭空消失。
 *        → 修法：造根的时候顺手把 key 存进去。
 *    (3) mergeChildren 里把「关键字个数 num_keys」当成「孩子下标」来用，
 *        还在循环里自增 → 数组越界写入，并且 num_keys 被写坏。
 *        → 修法：孩子下标改用合并前的基点 base + i 单独计算。
 *    (4) 删除内部节点的关键字时，原版只把前驱/后继的值搬上去就结束，
 *        没有真正递归删除那个前驱/后继 → 子树可能只剩不到 MIN_KEYS 个关键字，
 *        破坏 B 树性质。
 *        → 修法：搬完之后递归调用 deleteKeyRec 把它删掉。
 *    (5) search 没有判空，对空树查找会解引用 NULL 而崩溃。
 *        → 修法：入口加 if (node == NULL) return false;
 *    (6) 新增 validateTree()，每次操作后自动校验上面 5 条性质，
 *        用运行结果来证明这棵树始终是合法 B 树。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>   /* INT_MIN / INT_MAX：校验时当开区间的两端边界使用 */

/* ======================== 1. 结构体与基础工具 ======================== */

#define M        4                   /* 阶：每个节点最多 M 个子节点 */
#define MAX_KEYS (M - 1)             /* 每个节点最多关键字数 = 3 */
#define MIN_KEYS ((M + 1) / 2 - 1)   /* 非根节点最少关键字数 = 1 */

typedef struct BTreeNode {
    int keys[MAX_KEYS];             /* 关键字数组，升序存放 */
    struct BTreeNode *children[M];  /* 子节点指针数组，实际只用到 num_keys+1 个 */
    int num_keys;                   /* 当前关键字个数 */
    bool is_leaf;                   /* true = 叶子节点（不用 children） */
} BTreeNode;

/*
 * 创建一个空节点。
 * is_leaf = true  → 叶子节点
 * is_leaf = false → 内部节点（之后要把孩子挂上去）
 */
BTreeNode *createNode(bool is_leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (node == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    for (int i = 0; i < M; i++) {
        node->children[i] = NULL;   /* 显式置空，避免野指针 */
    }
    return node;
}

/* ------------------------------ 前向声明 ------------------------------
 * C 编译器是自上而下单遍扫描的。如果函数 A 里调用了后面才定义的 B，
 * 编译到那一行时编译器还不认识 B，就会按老规矩猜成 "int B()"；
 * 等读到真正的定义发现类型对不上，于是报：
 *     warning: implicit declaration of function 'B'
 *     error:   conflicting types for 'B'
 * 把函数原型提前写在这里，编译器就不再瞎猜了，问题消失。
 */
bool  search(BTreeNode *node, int key, BTreeNode **out_node, int *out_index);
BTreeNode *insert(BTreeNode *root, int key);
void  insertNonFull(BTreeNode *node, int key);
void  splitChild(BTreeNode *parent, int index, BTreeNode *child);
BTreeNode *deleteKey(BTreeNode *root, int key);
void  deleteKeyRec(BTreeNode *node, int key);
void  removeFromLeaf(BTreeNode *node, int index);
void  removeFromNonLeaf(BTreeNode *node, int index);
void  fillChild(BTreeNode *parent, int index);
void  borrowFromLeft(BTreeNode *parent, int index);
void  borrowFromRight(BTreeNode *parent, int index);
void  mergeChildren(BTreeNode *parent, int index);
int   getPredecessor(BTreeNode *node, int index);
int   getSuccessor(BTreeNode *node, int index);
bool  validateTree(BTreeNode *root);
bool  validateSubtree(BTreeNode *node, bool is_root, int lower, int upper,
                      int depth, int *leaf_depth);
int   countKeys(BTreeNode *node);
int   treeHeight(BTreeNode *node);
void  inorderTraversal(BTreeNode *node);
void  printNodeKeys(BTreeNode *node);
void  printTree(BTreeNode *node, int depth);
void  destroyTree(BTreeNode *node);

/* ============================ 2. 查找 ============================ */

/*
 * 在以 node 为根的子树中查找 key。
 *
 * 返回 true 表示找到；此时通过 out_node / out_index 带回
 * 「在哪个节点的第几个关键字」。
 * out_node / out_index 允许传 NULL，表示不关心这个信息。
 *
 * 查找过程：先在当前节点内从左往右线性扫描，找到第一个 >= key 的位置 i；
 *   - 若 keys[i] == key  → 命中
 *   - 若是叶子        → 说明树里没有
 *   - 否则            → key 只可能落在 children[i] 这棵子树里，递归下去
 */
bool search(BTreeNode *node, int key, BTreeNode **out_node, int *out_index) {
    if (node == NULL) return false;         /* 空树，或走到了空孩子 */

    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) i++;

    if (i < node->num_keys && key == node->keys[i]) {
        if (out_node)  *out_node  = node;   /* 把命中位置回传给调用者 */
        if (out_index) *out_index = i;
        return true;
    }

    if (node->is_leaf) return false;        /* 叶子都没找到 → 不存在 */

    return search(node->children[i], key, out_node, out_index);
}

/* ============================ 3. 插入 ============================ */

/*
 * 分裂 parent 的第 index 个孩子。
 * 前提：child 是满的（num_keys == MAX_KEYS）。
 *
 * 做法：把 child 的「中间关键字」上移给 parent，
 *       左半部分留在 child，右半部分装进新建的 newNode，
 *       最后把 newNode 挂到 parent->children[index + 1]。
 *
 * 示意（MAX_KEYS = 3，把 [1 2 3] 分裂，中间关键字 2 上移）：
 *
 *      parent: [10]                      parent: [2 10]
 *        /      \            ==>          /    |     \
 *   child:[1 2 3]  ...            child:[1]  newNode:[3]  ...
 *
 * 注意：分裂是「向上长」的，只影响父节点，不会破坏叶子的层高，
 *       这正是 B 树能始终保持平衡的原因。
 */
void splitChild(BTreeNode *parent, int index, BTreeNode *child) {
    BTreeNode *newNode = createNode(child->is_leaf);

    /* mid 是中间关键字的下标。MAX_KEYS=3 时 mid=1，即 keys[1] 上移 */
    int mid = MAX_KEYS / 2;

    /* ① child 右半边的关键字搬进 newNode
     *    （个数 = MAX_KEYS - mid - 1，3 个关键字时是 1 个） */
    newNode->num_keys = MAX_KEYS - mid - 1;
    for (int i = 0; i < newNode->num_keys; i++) {
        newNode->keys[i] = child->keys[mid + 1 + i];
    }

    /* ② 如果 child 不是叶子，它的孩子也要按同样比例分一半过去
     *    关键字的个数比孩子少 1，所以右半边孩子有 newNode->num_keys + 1 个 */
    if (!child->is_leaf) {
        for (int i = 0; i <= newNode->num_keys; i++) {
            newNode->children[i] = child->children[mid + 1 + i];
        }
    }

    /* ③ child 只保留左半边 */
    child->num_keys = mid;

    /* ④ 在 parent 里给 newNode 腾出孩子位置，再把它挂上 */
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = newNode;

    /* ⑤ 在 parent 里给中间关键字腾出位置，然后把中间关键字上移 */
    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
    }
    parent->keys[index] = child->keys[mid];
    parent->num_keys++;
}

/*
 * 往「还没满」的节点里插入 key。
 *
 * 思路：一路向下走，每遇到一个「已经满了」的孩子就先分裂它，
 *       这样递归下去时目标节点永远不是满的，插入绝不会溢出。
 *
 * 前提：调用时 node->num_keys < MAX_KEYS
 */
void insertNonFull(BTreeNode *node, int key) {
    int i = node->num_keys - 1;        /* 从右往左找插入位置 */

    if (node->is_leaf) {
        /* ---- 叶子节点：把比 key 大的关键字整体右移，再把 key 放进去 ---- */
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->num_keys++;
        return;
    }

    /* ---- 内部节点：先确定该走哪个孩子 ---- */
    while (i >= 0 && key < node->keys[i]) i--;
    i++;                               /* 现在 children[i] 是应该进入的子树 */

    /* 目标孩子已经满了 → 先分裂它，分裂后 node 自己会多出一个关键字 */
    if (node->children[i]->num_keys == MAX_KEYS) {
        splitChild(node, i, node->children[i]);
        /* 分裂后中间关键字升到了 node->keys[i]：
         * 如果 key 比它还大，说明该走它右边那个新节点 */
        if (key > node->keys[i]) i++;
    }

    insertNonFull(node->children[i], key);
}

/*
 * 插入入口。
 *
 * 空树：造一个叶子当根，并把 key 直接存进去。
 *       （原版这里漏了存 key，是最致命的 bug）
 *
 * 根已满：整棵树没法靠「分裂孩子」解决，必须先造一个新根，
 *         把旧根当成它的第 0 个孩子分裂一次 —— 树高 +1。
 *         这是 B 树唯一长高的方式，也是「所有叶子同层」能保持的原因。
 */
BTreeNode *insert(BTreeNode *root, int key) {
    /* 空树 */
    if (root == NULL) {
        root = createNode(true);
        root->keys[0] = key;
        root->num_keys = 1;
        return root;
    }

    /* 本演示假设关键字唯一，重复的直接忽略 */
    if (search(root, key, NULL, NULL)) {
        printf("Key %d already exists, insertion ignored\n", key);
        return root;
    }

    /* 根满了 → 先长高一层 */
    if (root->num_keys == MAX_KEYS) {
        BTreeNode *newRoot = createNode(false);
        newRoot->children[0] = root;    /* 旧根成为新根的第 0 个孩子 */
        splitChild(newRoot, 0, root);   /* 旧根分裂，中间关键字升到新根 */
        insertNonFull(newRoot, key);    /* 新根此时一定不满，正常插入 */
        return newRoot;
    }

    insertNonFull(root, key);
    return root;
}

/* ============================ 4. 删除 ============================ */

/*
 * 取 node->keys[index] 的「前驱」：
 * 左子树里最靠右（也就是最大）的那个关键字。
 */
int getPredecessor(BTreeNode *node, int index) {
    BTreeNode *cur = node->children[index];
    while (!cur->is_leaf) {
        cur = cur->children[cur->num_keys];   /* 一路走最右边的孩子 */
    }
    return cur->keys[cur->num_keys - 1];      /* 最右边的关键字 */
}

/*
 * 取 node->keys[index] 的「后继」：
 * 右子树里最靠左（也就是最小）的那个关键字。
 */
int getSuccessor(BTreeNode *node, int index) {
    BTreeNode *cur = node->children[index + 1];
    while (!cur->is_leaf) {
        cur = cur->children[0];               /* 一路走最左边的孩子 */
    }
    return cur->keys[0];
}

/*
 * 从叶子节点里删掉第 index 个关键字。
 * 叶子上删除不会破坏平衡，把后面的关键字整体左移即可。
 */
void removeFromLeaf(BTreeNode *node, int index) {
    for (int i = index; i < node->num_keys - 1; i++) {
        node->keys[i] = node->keys[i + 1];
    }
    node->num_keys--;
}

/*
 * 要删的关键字位于「内部节点」里。
 *
 * B 树不能直接从内部节点抠掉一个关键字（会破坏分隔结构和孩子数量），
 * 标准做法是三选一：
 *
 *   ① 左孩子富裕（关键字数 > MIN_KEYS）
 *        → 用「前驱」顶替它，然后递归去左孩子里把前驱真删掉
 *   ② 否则右孩子富裕
 *        → 用「后继」顶替它，然后递归去右孩子里把后继真删掉
 *   ③ 两边都不富裕
 *        → 把「左孩子 + 该关键字 + 右孩子」合并成一个节点，再递归删
 *
 * （原版只做了「把前驱/后继搬上来」，没有递归删除，会留下不合法的子节点）
 */
void removeFromNonLeaf(BTreeNode *node, int index) {
    int key = node->keys[index];

    if (node->children[index]->num_keys > MIN_KEYS) {
        /* 情况①：左孩子富裕 */
        int pred = getPredecessor(node, index);
        node->keys[index] = pred;                  /* 前驱顶替 */
        deleteKeyRec(node->children[index], pred); /* 递归删掉原来的前驱 */

    } else if (node->children[index + 1]->num_keys > MIN_KEYS) {
        /* 情况②：右孩子富裕 */
        int succ = getSuccessor(node, index);
        node->keys[index] = succ;                      /* 后继顶替 */
        deleteKeyRec(node->children[index + 1], succ); /* 递归删掉原来的后继 */

    } else {
        /* 情况③：两边都只有 MIN_KEYS 个关键字，合并成一个节点 */
        mergeChildren(node, index);
        deleteKeyRec(node->children[index], key);  /* 到合并后的节点里继续删 */
    }
}

/*
 * 把 children[index]、keys[index]、children[index+1] 三部分合并成左边那一个。
 *
 * 前提：两个孩子都只有 MIN_KEYS 个关键字，
 *       合并后正好是 MIN_KEYS + 1 + MIN_KEYS = MAX_KEYS 个，刚好不溢出。
 *
 * 合并完成后：右孩子被释放，父节点少一个关键字、少一个孩子。
 */
void mergeChildren(BTreeNode *parent, int index) {
    BTreeNode *left  = parent->children[index];
    BTreeNode *right = parent->children[index + 1];

    int base = left->num_keys;   /* 记下合并前 left 的关键字个数，后面算下标要用 */

    /* ① 父节点的分隔关键字下移到 left 的末尾 */
    left->keys[base] = parent->keys[index];
    left->num_keys = base + 1;

    /* ② right 的关键字全部追加到 left 后面 */
    for (int i = 0; i < right->num_keys; i++) {
        left->keys[left->num_keys] = right->keys[i];
        left->num_keys++;
    }

    /* ③ 不是叶子的话，right 的孩子指针也要接过来。
     *    left 原本有 base 个关键字、base+1 个孩子（下标 0..base），
     *    新增孩子从下标 base+1 开始排。
     *    ⚠️ 这里必须用 base+1+i 算下标，绝不能拿 left->num_keys 当孩子下标
     *       （原版就是在这儿把 children[] 写出了数组边界） */
    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++) {
            left->children[base + 1 + i] = right->children[i];
        }
    }

    /* ④ 父节点里删掉被下移的关键字，以及 right 这个孩子位置 */
    for (int i = index; i < parent->num_keys - 1; i++) {
        parent->keys[i] = parent->keys[i + 1];
        parent->children[i + 1] = parent->children[i + 2];
    }
    parent->num_keys--;

    free(right);   /* right 的内容已经并入 left，释放掉，避免内存泄漏 */
}

/*
 * 从左兄弟借一个关键字给 children[index]（父节点的分隔关键字做中转）。
 *
 * 本质是一次「旋转」：
 *        父: [ ...,  K, ... ]              父: [ ..., K', ... ]
 *             /         \        ==>           /        \
 *      左兄弟:[... a]   child:[b ...]   左兄弟:[...]   child:[K b ...]
 *   其中 K' 是左兄弟最大的关键字，K 是原父关键字。
 */
void borrowFromLeft(BTreeNode *parent, int index) {
    BTreeNode *child   = parent->children[index];
    BTreeNode *sibling = parent->children[index - 1];

    /* ① child 的关键字整体右移一位，腾出最前面的位置 */
    for (int i = child->num_keys; i > 0; i--) {
        child->keys[i] = child->keys[i - 1];
    }
    /* 孩子指针也必须右移，否则「关键字—孩子」的对应关系就全乱了 */
    if (!child->is_leaf) {
        for (int i = child->num_keys + 1; i > 0; i--) {
            child->children[i] = child->children[i - 1];
        }
    }

    /* ② 父节点的分隔关键字下移到 child 的开头 */
    child->keys[0] = parent->keys[index - 1];
    child->num_keys++;

    /* ③ 左兄弟最大的关键字上移到父节点 */
    parent->keys[index - 1] = sibling->keys[sibling->num_keys - 1];

    /* ④ 左兄弟最后那个孩子过继给 child，当它的第一个孩子
     *    （关键字个数比孩子少 1，落在最后一个关键字右边的孩子下标就是 num_keys） */
    if (!child->is_leaf) {
        child->children[0] = sibling->children[sibling->num_keys];
        sibling->children[sibling->num_keys] = NULL;
    }

    sibling->num_keys--;   /* 左兄弟少了一个关键字 */
}

/*
 * 从右兄弟借一个关键字给 children[index]，是 borrowFromLeft 的镜像操作。
 */
void borrowFromRight(BTreeNode *parent, int index) {
    BTreeNode *child   = parent->children[index];
    BTreeNode *sibling = parent->children[index + 1];

    /* ① 父节点的分隔关键字下移到 child 的末尾 */
    child->keys[child->num_keys] = parent->keys[index];
    child->num_keys++;

    /* ② 右兄弟最小的关键字上移到父节点 */
    parent->keys[index] = sibling->keys[0];

    /* ③ 右兄弟的关键字整体左移一位 */
    for (int i = 0; i < sibling->num_keys - 1; i++) {
        sibling->keys[i] = sibling->keys[i + 1];
    }

    /* ④ 右兄弟最左边那个孩子过继给 child，放在它的末尾
     *    child 现在有 num_keys 个关键字，末尾孩子下标正好是 num_keys */
    if (!child->is_leaf) {
        child->children[child->num_keys] = sibling->children[0];
        for (int i = 0; i < sibling->num_keys - 1; i++) {
            sibling->children[i] = sibling->children[i + 1];
        }
        sibling->children[sibling->num_keys - 1] = NULL;
    }

    sibling->num_keys--;   /* 右兄弟少了一个关键字 */
}

/*
 * 保证 parent->children[index] 至少有 MIN_KEYS + 1 个关键字。
 *
 * 为什么要在「进去之前」就补足？
 * 因为删除会从这棵子树里拿走一个关键字。如果进去时它只有 MIN_KEYS 个，
 * 删完就只剩 MIN_KEYS - 1 个，违反 B 树性质。
 * 所以标准做法是：往哪个孩子走，就先把那个孩子喂饱。
 *
 * 三种手段，优先「借」（对树结构影响小），借不到才「合并」：
 *   ① 左兄弟富裕 → 从左借
 *   ② 右兄弟富裕 → 从右借
 *   ③ 都不富裕   → 合并
 */
void fillChild(BTreeNode *parent, int index) {
    if (index > 0 && parent->children[index - 1]->num_keys > MIN_KEYS) {
        borrowFromLeft(parent, index);

    } else if (index < parent->num_keys &&
               parent->children[index + 1]->num_keys > MIN_KEYS) {
        borrowFromRight(parent, index);

    } else {
        /* 两边都借不到 → 只能合并。
         * 如果 index 正好是最后一个孩子，就改成和左兄弟合并。 */
        if (index < parent->num_keys) {
            mergeChildren(parent, index);
        } else {
            mergeChildren(parent, index - 1);
        }
    }
}

/*
 * 删除的递归主体：从以 node 为根的子树里删掉 key。
 *
 * 整体结构和查找类似，一路向下：
 *   1. 先算出 key 应该落在哪个位置 / 哪个孩子
 *   2. 如果 key 就在当前节点里 → 按「叶子 / 内部节点」分别处理
 *   3. 如果 key 在下面的子树里 → 先把那个孩子喂饱（fillChild），再递归进去
 */
void deleteKeyRec(BTreeNode *node, int key) {
    if (node == NULL) return;

    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) i++;

    /* ---------- 情况 A：key 就在当前节点里 ---------- */
    if (i < node->num_keys && node->keys[i] == key) {
        if (node->is_leaf) {
            removeFromLeaf(node, i);        /* 叶子：直接删，不影响平衡 */
        } else {
            removeFromNonLeaf(node, i);     /* 内部：前驱/后继顶替，或合并 */
        }
        return;
    }

    /* ---------- 情况 B：key 在下面的子树里 ---------- */
    if (node->is_leaf) {
        return;                             /* 叶子都没找到 → key 不在树中 */
    }

    /* 记下「是不是落在最后一个孩子」，后面合并会改变下标，需要用它来判断 */
    bool is_last_child = (i == node->num_keys);

    /* 关键一步：进孩子之前先把它喂饱 */
    if (node->children[i]->num_keys == MIN_KEYS) {
        fillChild(node, i);

        /* fillChild 可能把 children[i] 和左兄弟合并掉了，
         * 这时 i 已经超出新的 num_keys 范围，应该往左挪一格继续 */
        if (is_last_child && i > node->num_keys) {
            deleteKeyRec(node->children[i - 1], key);
            return;
        }
    }

    deleteKeyRec(node->children[i], key);
}

/*
 * 删除入口。
 *
 * 特殊情况：如果删完根节点变成「没有关键字的内部节点」，
 * 说明树的高度该降了 —— 把它唯一的孩子提拔为新根，再释放旧根。
 * 这是 B 树唯一降低树高的方式。
 */
BTreeNode *deleteKey(BTreeNode *root, int key) {
    if (root == NULL) {
        printf("Tree is empty, cannot delete %d\n", key);
        return NULL;
    }

    /* 先查一遍，好给使用者一个明确反馈 */
    if (!search(root, key, NULL, NULL)) {
        printf("Key %d is not in the tree\n", key);
        return root;
    }

    deleteKeyRec(root, key);

    /* 根空了但还有孩子 → 降低树高 */
    if (root->num_keys == 0 && !root->is_leaf) {
        BTreeNode *newRoot = root->children[0];
        free(root);
        root = newRoot;
    }
    return root;
}

/* ====================== 5. B 树性质校验 ====================== */

/*
 * 递归校验以 node 为根的子树是否满足 B 树性质。
 *
 * 参数：
 *   is_root    是否是整棵树的根（对「最少关键字数」的要求更宽松）
 *   lower,upper这棵子树里所有关键字必须严格落在开区间 (lower, upper) 内
 *   depth      当前深度，用来检查「所有叶子同层」
 *   leaf_depth 记录遇到的第一个叶子的深度；之后的叶子必须与它相同
 *
 * 返回：这棵子树是否合法
 */
bool validateSubtree(BTreeNode *node, bool is_root, int lower, int upper,
                     int depth, int *leaf_depth) {
    if (node == NULL) return true;

    bool ok = true;

    /* 性质 1 & 2 & 3：关键字个数必须在允许范围内 */
    int min_allowed = is_root ? 1 : MIN_KEYS;
    if (node->num_keys < min_allowed || node->num_keys > MAX_KEYS) {
        printf("  [x] key count %d is out of range [%d, %d]\n",
               node->num_keys, min_allowed, MAX_KEYS);
        ok = false;
    }

    /* 性质 4：节点内严格升序，并且都在限定区间内 */
    for (int i = 0; i < node->num_keys; i++) {
        if (i > 0 && node->keys[i - 1] >= node->keys[i]) {
            printf("  [x] keys inside a node are not strictly increasing\n");
            ok = false;
        }
        if (node->keys[i] <= lower || node->keys[i] >= upper) {
            printf("  [x] key %d is outside its subtree range (%d, %d)\n",
                   node->keys[i], lower, upper);
            ok = false;
        }
    }

    /* 性质 5：所有叶子必须在同一层 */
    if (node->is_leaf) {
        if (*leaf_depth == -1) {
            *leaf_depth = depth;                 /* 记下第一片叶子的深度 */
        } else if (*leaf_depth != depth) {
            printf("  [x] leaves are not on the same level: depth %d vs %d\n",
                   depth, *leaf_depth);
            ok = false;
        }
        return ok;
    }

    /* 非叶子节点：孩子数必须等于关键字数 + 1，且每个孩子递归检查 */
    for (int i = 0; i <= node->num_keys; i++) {
        if (node->children[i] == NULL) {
            printf("  [x] internal node child %d is NULL (expected %d children)\n",
                   i, node->num_keys + 1);
            ok = false;
            continue;
        }
        /* 每个孩子允许的关键字范围，由父节点的关键字夹出来 */
        int child_lower = (i == 0) ? lower : node->keys[i - 1];
        int child_upper = (i == node->num_keys) ? upper : node->keys[i];

        if (!validateSubtree(node->children[i], false,
                             child_lower, child_upper, depth + 1, leaf_depth)) {
            ok = false;
        }
    }
    return ok;
}

/* 对外入口：整棵树是不是一棵合法的 B 树 */
bool validateTree(BTreeNode *root) {
    if (root == NULL) return true;         /* 空树视为合法 */
    int leaf_depth = -1;
    return validateSubtree(root, true, INT_MIN, INT_MAX, 0, &leaf_depth);
}

/* 统计整棵树的关键字总数 */
int countKeys(BTreeNode *node) {
    if (node == NULL) return 0;
    int total = node->num_keys;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            total += countKeys(node->children[i]);
        }
    }
    return total;
}

/*
 * 求树高（根所在层记为 1，空树为 0）。
 * 因为 B 树所有叶子同层，所以随便走一条最左边的路径就是树高。
 */
int treeHeight(BTreeNode *node) {
    if (node == NULL) return 0;
    int h = 1;
    while (!node->is_leaf) {
        node = node->children[0];
        h++;
    }
    return h;
}

/* ==================== 6. 遍历、打印、销毁 ==================== */

/*
 * 中序遍历。
 * B 树的中序输出就是「把所有关键字从小到大打印一遍」，
 * 用它来验证有序性最直观。
 */
void inorderTraversal(BTreeNode *node) {
    if (node == NULL) return;
    for (int i = 0; i < node->num_keys; i++) {
        inorderTraversal(node->children[i]);
        printf("%d ", node->keys[i]);
    }
    inorderTraversal(node->children[node->num_keys]);
}

/* 打印单个节点的关键字，形如 [10 20 30] */
void printNodeKeys(BTreeNode *node) {
    printf("[");
    for (int i = 0; i < node->num_keys; i++) {
        if (i > 0) printf(" ");
        printf("%d", node->keys[i]);
    }
    printf("]");
}

/* 按缩进打印整棵树，直观看出层次结构（缩进越深，层级越深） */
void printTree(BTreeNode *node, int depth) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("        ");
    printNodeKeys(node);
    if (node->is_leaf) printf("   <- leaf");
    printf("\n");

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            printTree(node->children[i], depth + 1);
        }
    }
}

/*
 * 递归释放整棵树。
 * 必须先递归释放孩子，再释放自己 —— 顺序反了就找不到孩子了。
 */
void destroyTree(BTreeNode *node) {
    if (node == NULL) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroyTree(node->children[i]);
        }
    }
    free(node);
}

/* ============================ 7. main 演示 ============================ */

/* -------------------- 自检小工具（与其它演示文件统一） -------------------- */

static int g_pass = 0;
static int g_fail = 0;

/* 记一次「只关心通过与否」的检查：每次性质校验都会调用它 */
static void recordCheck(bool ok) {
    if (ok) g_pass++; else g_fail++;
}

/* 比对实测值与预期值 */
static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-36s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-36s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

static void expectBool(const char *what, bool got, bool want) {
    printf("  [%s] %-36s = %s\n", got == want ? "ok  " : "FAIL",
           what, got ? "yes" : "no");
    if (got == want) g_pass++; else g_fail++;
}

/* 把中序结果收进数组（inorderTraversal 只负责打印，自检需要拿到序列） */
static int collectInorder(BTreeNode *node, int *out, int idx) {
    if (node == NULL) return idx;
    for (int i = 0; i < node->num_keys; i++) {
        idx = collectInorder(node->children[i], out, idx);
        out[idx++] = node->keys[i];
    }
    return collectInorder(node->children[node->num_keys], out, idx);
}

/* 中序序列是否严格升序（B 树的关键字必须有序） */
static bool inorderAscending(BTreeNode *root, int count) {
    int buf[256];
    if (count > (int)(sizeof(buf) / sizeof(buf[0]))) return false;
    if (collectInorder(root, buf, 0) != count) return false;
    for (int i = 1; i < count; i++) {
        if (buf[i - 1] >= buf[i]) return false;
    }
    return true;
}

/* 打印当前树的形状 + 统计信息 + 性质校验结果 */
void showState(BTreeNode *root, const char *title) {
    printf("-------- %s --------\n", title);

    if (root == NULL) {
        printf("(empty tree)\n\n");
        return;
    }

    printTree(root, 0);
    printf("Total keys: %d    Height: %d    Root keys: %d\n",
           countKeys(root), treeHeight(root), root->num_keys);

    printf("Inorder: ");
    inorderTraversal(root);
    printf("\n");

    bool ok = validateTree(root);
    recordCheck(ok);                       /* 每一步的性质校验都计入最终汇总 */
    printf("B-tree property check (5 rules): %s\n\n", ok ? "PASS" : "FAIL");
}

int main(void) {
    BTreeNode *root = NULL;

    printf("===========================================================\n");
    printf("  %d-order B-tree demo (max %d keys per node, min %d for non-root)\n",
           M, MAX_KEYS, MIN_KEYS);
    printf("===========================================================\n\n");

    /* ---------------- 插入测试 ---------------- */
    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 25};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    printf("[INSERT] inserting in order: ");
    for (int i = 0; i < n; i++) printf("%d ", keys[i]);
    printf("\n\n");

    for (int i = 0; i < n; i++) {
        root = insert(root, keys[i]);
        char title[64];
        snprintf(title, sizeof(title), "after inserting %d", keys[i]);
        showState(root, title);
    }

    /* ---------------- 插入后的自检 ---------------- */
    printf("[SELF CHECK after all inserts]\n");
    expectInt("key count", countKeys(root), 10);
    expectInt("height",    treeHeight(root), 3);
    expectBool("all 5 properties hold", validateTree(root), true);
    expectBool("inorder is ascending",  inorderAscending(root, 10), true);
    {
        BTreeNode *found_node = NULL;
        int found_index = -1;
        int hit = 0;
        for (int i = 0; i < n; i++) {
            if (search(root, keys[i], &found_node, &found_index)) hit++;
        }
        expectInt("every inserted key is findable", hit, 10);
        expectBool("searching a missing key fails",
                   search(root, 99, &found_node, &found_index), false);
    }
    printf("\n");

    /* ---------------- 查找测试 ---------------- */
    printf("[SEARCH]\n");
    int query[] = {6, 15, 30, 99};
    for (int i = 0; i < 4; i++) {
        BTreeNode *found_node = NULL;
        int found_index = -1;

        if (search(root, query[i], &found_node, &found_index)) {
            printf("  %2d -> found in node ", query[i]);
            printNodeKeys(found_node);
            printf(" at index %d (0-based)\n", found_index);
        } else {
            printf("  %2d -> not found\n", query[i]);
        }
    }
    printf("\n");

    /* ---------------- 删除测试 ---------------- */
    int del[] = {1, 12, 17, 10, 5, 20};
    int dn = (int)(sizeof(del) / sizeof(del[0]));

    printf("[DELETE] deleting in order: ");
    for (int i = 0; i < dn; i++) printf("%d ", del[i]);
    printf("\n");
    printf("(exercises borrow from predecessor/successor, node merge, and tree shrink)\n\n");

    for (int i = 0; i < dn; i++) {
        root = deleteKey(root, del[i]);
        char title[64];
        snprintf(title, sizeof(title), "after deleting %d", del[i]);
        showState(root, title);
    }

    /* ---------------- 删除后的自检 ---------------- */
    printf("[SELF CHECK after all deletes]\n");
    expectInt("remaining key count", countKeys(root), 5);
    expectInt("height",              treeHeight(root), 2);
    expectBool("all 5 properties still hold", validateTree(root), true);
    expectBool("inorder is still ascending",  inorderAscending(root, 5), true);
    {
        BTreeNode *found_node = NULL;
        int found_index = -1;
        expectBool("deleted key 12 is gone",      search(root, 12, &found_node, &found_index), false);
        expectBool("deleted key 20 is gone",      search(root, 20, &found_node, &found_index), false);
        expectBool("deleted key 1 is gone",       search(root,  1, &found_node, &found_index), false);
        expectBool("remaining key 25 is there",   search(root, 25, &found_node, &found_index), true);
        expectBool("remaining key 3 is there",    search(root,  3, &found_node, &found_index), true);
    }

    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- 收尾 ---------------- */
    printf("[FREE]\n");
    destroyTree(root);
    printf("The whole tree has been freed\n");

    return 0;
}
