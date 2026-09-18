/*
 * ============================================================================
 *  多路平衡搜索树（B 树）完整演示
 * ============================================================================
 *
 *  【命名说明】
 *  本文件名沿用了 B+tree.c，但里面的结构是标准 **B 树**：
 *  关键字既出现在内部节点，也出现在叶子节点。
 *  真正的 B+ 树要求「所有关键字只存放在叶子层，并把叶子串成链表」，
 *  那是数据库索引的常见做法（范围查询更快）。本文件演示的是 B 树本身。
 *
 *  【一、B 树是什么】
 *  它是 BST 的「多路」推广：一个节点不是「1 个关键字 + 2 个孩子」，
 *  而是「k 个有序关键字 + k+1 个孩子」。
 *  M 阶 B 树（本文件 M = 4）必须满足：
 *        ① 每个节点最多 M-1 个关键字（本文件 3 个）
 *        ② 根节点至少有 1 个关键字
 *        ③ 其他节点至少有 ⌈M/2⌉-1 个关键字（本文件 1 个）
 *        ④ 节点内关键字严格升序，且左边子树全小、右边子树全大（多路版 BST 性质）
 *        ⑤ 所有叶子都在同一层（完美平衡 —— B 树从不需要「旋转」）
 *
 *  【二、B 树靠什么保持平衡】
 *  BST / AVL / 红黑树靠「旋转」，B 树靠「分裂」和「合并」：
 *        - 插入：自底向上分裂，满节点一拆为二，中间关键字上移给父节点
 *        - 删除：孩子关键字不足（下溢）时先向兄弟借，借不到就与兄弟合并
 *  这不是为了炫技：B 树的真实用途是磁盘与数据库索引 —— 节点大、树矮，
 *  一次 I/O 读一整个节点，树高（= 磁盘访问次数）能压到 3~4 层。
 *
 *  【三、文件结构】
 *    1. 类型与常量      BTreeNode / M / MAX_KEYS / MIN_KEYS
 *    2. 创建与查找      createNode / search
 *    3. 分裂与插入      splitChild / insertNonFull / insert
 *    4. 删除            removeFromLeaf / removeFromNonLeaf / merge /
 *                       borrowFromLeft / borrowFromRight / deleteKey
 *    5. 遍历与打印      inorderTraversal / printTree
 *    6. 校验与释放      validateTree / countKeys / destroyTree
 *    7. main 演示       插入 / 查找 / 删除 / 随机增删压力测试
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>    /* INT_MIN / INT_MAX，校验子树区间时当边界用 */

#define M 4                          /* B 树的阶：一个节点最多 M 个孩子 */
#define MAX_KEYS (M - 1)             /* 每个节点最多关键字数 = 3 */
#define MIN_KEYS ((M + 1) / 2 - 1)   /* 非根节点最少关键字数 = 1 */

/* B 树节点 */
typedef struct BTreeNode {
    int keys[MAX_KEYS];              /* 关键字数组（升序存放） */
    struct BTreeNode *children[M];   /* 子节点指针数组（叶子节点用不到） */
    int num_keys;                    /* 当前关键字数量 */
    bool is_leaf;                    /* 是否为叶子节点 */
} BTreeNode;

/* ------------------------------ 前向声明 ------------------------------
 * C 编译器自上而下单遍扫描，函数必须先声明后使用。
 * 下面这些函数互相调用（removeFromNonLeaf 要用 merge 和 deleteKey），
 * 若不先声明，编译器会按「隐式声明」处理，接着报 conflicting types 错误 ——
 * 这正是本文件原先编译不过的原因。
 */
BTreeNode *createNode(bool is_leaf);
bool       search(BTreeNode *node, int key, int *pos);
void       splitChild(BTreeNode *parent, int index, BTreeNode *child);
void       insertNonFull(BTreeNode *node, int key);
BTreeNode *insert(BTreeNode *root, int key);
void       removeFromLeaf(BTreeNode *node, int index);
void       removeFromNonLeaf(BTreeNode *node, int index);
void       merge(BTreeNode *node, int index);
void       borrowFromRight(BTreeNode *node, int index);
void       borrowFromLeft(BTreeNode *node, int index);
BTreeNode *deleteKey(BTreeNode *node, int key);
void       inorderTraversal(BTreeNode *node);
void       printTree(BTreeNode *node, int level);
bool       validateTree(BTreeNode *root);
int        countKeys(BTreeNode *node);
void       destroyTree(BTreeNode *node);

/* 创建新节点（关键字个数为 0，所有孩子指针置空） */
BTreeNode *createNode(bool is_leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (!node) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    for (int i = 0; i < M; i++) {
        node->children[i] = NULL;
    }
    return node;
}

/*
 * 查找关键字。
 * 从根往下走：先在节点内顺序比较，找到就返回位置；
 * 没找到就顺着「第 i 个孩子」继续往下（i 是第一个比 key 大的关键字的下标）。
 * 成功返回 true，并把下标写入 *pos；失败返回 false。
 */
bool search(BTreeNode *node, int key, int *pos) {
    if (node == NULL) return false;      /* ★ 空树查找：别解引用 NULL */

    int i = 0;
    while (i < node->num_keys && key > node->keys[i]) {
        i++;
    }
    if (i < node->num_keys && key == node->keys[i]) {
        *pos = i;                        /* 命中，记下是第几个关键字 */
        return true;
    }
    if (node->is_leaf) {
        return false;                    /* 走到叶子还没找到 = 树上没有 */
    }
    return search(node->children[i], key, pos);
}

/*
 * 分裂满子节点 —— B 树「长高」的唯一途径。
 *
 * 前提：child 已经满了（num_keys == MAX_KEYS == 3），parent 还没满。
 * 做法：把 child 一分为二，中间那个关键字「提」到父节点里当分界。
 *
 *        parent:  [ ... child ... ]              parent: [ ... mid ... ]
 *                    |                                        /        \
 *        child:  [a b c]  (满)        ==>      [a]                    [c]
 *                / | | \                       /   \                 /   \
 *              0  1 2  3                     0     1               2     3
 *
 * 参数：parent 父节点，index 满子节点在父节点中的下标，child 满子节点。
 * 注意本函数不改 child 里 keys[mid] 的值，它只是被复制到父节点；
 * 原来那个位置随后会被 num_keys 的缩小「抛弃」，不再参与比较。
 */
void splitChild(BTreeNode *parent, int index, BTreeNode *child) {
    BTreeNode *newNode = createNode(child->is_leaf);
    int mid = MAX_KEYS / 2;  // 中间关键字的索引（对于 4 阶树 mid = 1）

    // 将 child 后半部分的关键字复制到 newNode
    newNode->num_keys = mid;
    for (int i = 0; i < mid; i++) {
        newNode->keys[i] = child->keys[i + mid + 1];
    }

    // 如果 child 不是叶子节点，子节点指针也要一并搬过去（后半段的 mid+1 个）
    if (!child->is_leaf) {
        for (int i = 0; i <= mid; i++) {
            newNode->children[i] = child->children[i + mid + 1];
        }
    }

    child->num_keys = mid;   // child 只留前半部分

    // 在父节点中为新节点腾出孩子指针的位置（从后往前挪，避免覆盖）
    for (int i = parent->num_keys; i > index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = newNode;

    // 在父节点中为中间关键字腾出位置（同样从后往前挪）
    for (int i = parent->num_keys - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
    }

    // 将中间关键字上移到父节点
    parent->keys[index] = child->keys[mid];
    parent->num_keys++;
}

/*
 * 向「还没满」的节点里插入关键字（前提：node->num_keys < MAX_KEYS）。
 *
 * 思路（《算法导论》的下行插入法）：一路往下走，
 * 遇到「准备下去的那个孩子已经满了」就先把它分裂，然后再下去。
 * 这样能保证进入任何节点时它都不满，插入永远发生在叶子层，不会回溯。
 */
void insertNonFull(BTreeNode *node, int key) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        // 叶子节点：从后往前把比 key 大的关键字右移，腾出位置
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->num_keys++;
    } else {
        // 内部节点：先确定应该走哪个孩子
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;

        // 要下去的那个孩子已经满了 → 先分裂它
        if (node->children[i]->num_keys == MAX_KEYS) {
            splitChild(node, i, node->children[i]);
            if (key > node->keys[i]) {
                i++;          // 中间关键字上移了，可能该走新分出来的兄弟
            }
        }
        insertNonFull(node->children[i], key);
    }
}

/*
 * 插入关键字（对外入口）。
 * 根节点满时要特殊处理：先造一个空的新根，再分裂旧根 ——
 * 这是 B 树「长高」的唯一途径（对比 AVL/红黑树：那两者靠旋转，不改变根的位置）。
 */
BTreeNode *insert(BTreeNode *root, int key) {
    if (root == NULL) {
        BTreeNode *first = createNode(true);
        insertNonFull(first, key);   /* ★ 第一个关键字也必须真的存进去 */
        return first;
    }

    // 如果根节点已满，需要分裂并创建新根
    if (root->num_keys == MAX_KEYS) {
        BTreeNode *newRoot = createNode(false);
        newRoot->children[0] = root;
        splitChild(newRoot, 0, root);
        insertNonFull(newRoot, key);
        return newRoot;
    }

    insertNonFull(root, key);
    return root;
}

/* 从叶子节点里移除第 index 个关键字（直接左移覆盖，最简单的一种删除） */
void removeFromLeaf(BTreeNode *node, int index) {
    for (int i = index; i < node->num_keys - 1; i++) {
        node->keys[i] = node->keys[i + 1];
    }
    node->num_keys--;
}

/*
 * 删除「内部节点」里的第 index 个关键字。
 * 内部节点的关键字不能直接摘掉，否则它的左右两棵子树就接不起来了。
 * 标准做法是找一个「替身」：
 *        ① 左孩子的关键字够多（>  MIN_KEYS）→ 用左子树里的最大值（前驱）顶上
 *        ② 否则右孩子够多               → 用右子树里的最小值（后继）顶上
 *        ③ 两边都刚好够（== MIN_KEYS）→ 把左、右孩子连同本关键字合并成一个节点，再往下删
 *
 * ★ 这里最容易写错的地方：搬完替身之后，必须**递归调用 deleteKey 真正删掉替身**，
 *   不能图省事直接 removeFromLeaf。因为替身所在的叶子删完之后可能低于下限（下溢），
 *   而下溢要靠 deleteKey 里的「借位 / 合并」来修 —— 直接 removeFromLeaf 会跳过这一步，
 *   留下一棵「关键字个数少于下限」的非法 B 树。
 *   同理，递归的返回值（子树根可能因合并而变化）必须接回父节点的孩子指针。
 */
void removeFromNonLeaf(BTreeNode *node, int index) {
    int key = node->keys[index];

    // 情况①：左子树关键字足够多，用前驱（左子树最大值）替代
    if (node->children[index]->num_keys > MIN_KEYS) {
        BTreeNode *pred = node->children[index];
        while (!pred->is_leaf) {
            pred = pred->children[pred->num_keys];   // 一路向右，走到叶子
        }
        int predKey = pred->keys[pred->num_keys - 1];
        node->keys[index] = predKey;                 // 只搬「值」，不搬节点
        // 递归下去把那个替身真正删掉（可能触发下溢修复）
        node->children[index] = deleteKey(node->children[index], predKey);
    }
    // 情况②：右子树关键字足够多，用后继（右子树最小值）替代
    else if (node->children[index + 1]->num_keys > MIN_KEYS) {
        BTreeNode *succ = node->children[index + 1];
        while (!succ->is_leaf) {
            succ = succ->children[0];                // 一路向左，走到叶子
        }
        int succKey = succ->keys[0];
        node->keys[index] = succKey;
        node->children[index + 1] = deleteKey(node->children[index + 1], succKey);
    }
    // 情况③：左右孩子都刚好到下限 → 三者合并成一个节点，再在合并后的节点里删
    else {
        merge(node, index);
        node->children[index] = deleteKey(node->children[index], key);
    }
}

/*
 * 合并父节点的第 index 个关键字与它的左右两个孩子（三者并成一个节点）。
 *
 *         node:  [ ... k ... ]                 node:  [ ... ]   (k 被摘走)
 *                 /     \                ==>         |
 *       left:[a]     right:[b]                  left:[a k b]
 *
 * 合并后左孩子一定不超上限，所以合并是安全的：
 *         left(≤ MIN) + 1 + right(≤ MIN) ≤ 2·MIN + 1 ≤ MAX_KEYS
 */
void merge(BTreeNode *node, int index) {
    BTreeNode *left = node->children[index];
    BTreeNode *right = node->children[index + 1];
    int midKey = node->keys[index];

    /* ★ 先记下合并前左孩子的关键字个数。
     * 「关键字个数」和「孩子下标」是两个不同的量：左孩子有 n 个关键字，
     * 它的孩子却有 n+1 个（下标 0..n）。混用这两个量，num_keys 就会被写坏 ——
     * 这正是本文件原实现里的错误。 */
    int leftKeys = left->num_keys;

    // ① 父节点的关键字下移到左子节点末尾
    left->keys[leftKeys] = midKey;

    // ② 右子节点的关键字依次接在后面
    for (int i = 0; i < right->num_keys; i++) {
        left->keys[leftKeys + 1 + i] = right->keys[i];
    }

    // ③ 合并后的关键字总数
    left->num_keys = leftKeys + 1 + right->num_keys;

    // ④ 若不是叶子，孩子指针也要接上（right 有 num_keys+1 个孩子）
    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++) {
            left->children[leftKeys + 1 + i] = right->children[i];
        }
    }

    // ⑤ 从父节点中移除被摘走的关键字和右孩子指针（整体左移）
    for (int i = index; i < node->num_keys - 1; i++) {
        node->keys[i] = node->keys[i + 1];
        node->children[i + 1] = node->children[i + 2];
    }
    node->num_keys--;
    free(right);                     /* 右孩子的内容已经全部搬空，可以释放 */
}

/*
 * 借位：从「右兄弟」借一个关键字过来（兄弟富余时才用这招，树形不变）。
 * 关键字不是直接搬家，而是绕一圈：
 *        父节点的分隔关键字下移到 child 末尾
 *        右兄弟的第一个关键字上移，补上父节点空出来的位置
 * 只借一个就够 —— 目标是让 child 回到下限，不需要合并节点。
 *
 *         node:  [ k ]                      node:  [ b ]
 *               /     \            ==>          /     \
 *      child:[a]   right:[b c]            child:[a k]  right:[c]
 */
void borrowFromRight(BTreeNode *node, int index) {
    BTreeNode *child = node->children[index];
    BTreeNode *rightSibling = node->children[index + 1];

    // ① 父节点的分隔关键字下移到 child 末尾
    child->keys[child->num_keys] = node->keys[index];
    child->num_keys++;

    // ② 右兄弟的第一个关键字上移到父节点
    node->keys[index] = rightSibling->keys[0];

    // ③ 右兄弟自己的关键字整体左移，补上空位
    for (int i = 0; i < rightSibling->num_keys - 1; i++) {
        rightSibling->keys[i] = rightSibling->keys[i + 1];
    }
    rightSibling->num_keys--;

    // ④ 若不是叶子，孩子指针也要跟着借一个过来
    if (!child->is_leaf) {
        child->children[child->num_keys] = rightSibling->children[0];

        /* 右兄弟的孩子整体左移：new[i] = old[i+1]。
         * ★ 循环次数是「孩子个数」= num_keys + 1（此时 num_keys 已经减过 1），
         *   不是 num_keys！少移一次就会把最后一个孩子直接丢掉 ——
         *   那可是一整棵子树（连同它的关键字）凭空消失。
         *   这是原实现里最隐蔽的一处错误：树不会立刻崩，只是悄悄少了一批数据。 */
        for (int i = 0; i <= rightSibling->num_keys; i++) {
            rightSibling->children[i] = rightSibling->children[i + 1];
        }
        rightSibling->children[rightSibling->num_keys + 1] = NULL;
    }
}

/*
 * 借位：从「左兄弟」借一个关键字过来（与上面完全镜像）。
 * 区别在方向：child 里的关键字要整体右移，腾出开头的空位。
 */
void borrowFromLeft(BTreeNode *node, int index) {
    BTreeNode *child = node->children[index];
    BTreeNode *leftSibling = node->children[index - 1];

    // ① child 的关键字整体右移，腾出开头位置
    for (int i = child->num_keys; i > 0; i--) {
        child->keys[i] = child->keys[i - 1];
    }
    // 孩子指针同样右移（注意是 num_keys+1 个）
    if (!child->is_leaf) {
        for (int i = child->num_keys + 1; i > 0; i--) {
            child->children[i] = child->children[i - 1];
        }
    }

    // ② 父节点的分隔关键字下移到 child 开头
    child->keys[0] = node->keys[index - 1];
    child->num_keys++;

    // ③ 左兄弟的最后一个关键字上移到父节点
    node->keys[index - 1] = leftSibling->keys[leftSibling->num_keys - 1];
    leftSibling->num_keys--;

    // ④ 若不是叶子，把左兄弟最后一个孩子挪到 child 开头
    if (!child->is_leaf) {
        child->children[0] = leftSibling->children[leftSibling->num_keys + 1];
        leftSibling->children[leftSibling->num_keys + 1] = NULL;
    }
}

/*
 * 删除关键字（对外入口，也是递归主体）。
 *
 * 分两大支：
 *   ① 关键字就在本节点里：
 *        - 本节点是叶子     → 直接删
 *        - 本节点是内部节点 → 交给 removeFromNonLeaf（换前驱/后继，或先合并）
 *   ② 关键字在下面的子树里：
 *        - 先看准备下去的那个孩子是不是「刚好到下限」：
 *              能向左兄弟借 → 借位（树形不变）
 *              能向右兄弟借 → 借位
 *              都借不到     → 与该兄弟合并
 *          这一步的意义是保证「下降进入的节点一定比下限富裕」，
 *          于是删除后不会立刻下溢，一趟下行就能完成，不需要回溯。
 *        - 然后递归下去删，并把返回值接回孩子指针
 *
 * 收尾：根被删空时收缩（让唯一的孩子当新根），树高减 1。
 */
BTreeNode *deleteKey(BTreeNode *node, int key) {
    if (!node) return NULL;

    int index = 0;
    while (index < node->num_keys && key > node->keys[index]) {
        index++;
    }

    // 找到关键字
    if (index < node->num_keys && node->keys[index] == key) {
        if (node->is_leaf) {
            // 情况1：叶子节点，直接删除
            removeFromLeaf(node, index);
        } else {
            // 情况2：内部节点，需要先找一个替身
            removeFromNonLeaf(node, index);
        }
    } else {
        // 关键字不在当前节点，应该落在 children[index] 这棵子树中
        if (node->is_leaf) {
            printf("  key %d is not in the tree\n", key);
            return node;
        }

        BTreeNode *child = node->children[index];

        // 如果准备下去的孩子刚好到下限，先借位或合并
        if (child->num_keys == MIN_KEYS) {
            if (index > 0 && node->children[index - 1]->num_keys > MIN_KEYS) {
                borrowFromLeft(node, index);          // 向左兄弟借
            } else if (index < node->num_keys && node->children[index + 1]->num_keys > MIN_KEYS) {
                borrowFromRight(node, index);         // 向右兄弟借
            } else if (index < node->num_keys) {
                merge(node, index);                   // 与右兄弟合并，child 下标不变
            } else {
                merge(node, index - 1);               // 与左兄弟合并
                index--;                              // ★ 合并后目标节点左移一位
                child = node->children[index];
            }
        }

        // ★ 把递归的返回值接回孩子指针。
        //   子树根可能因为「下层发生合并」而变化；丢掉返回值会让本节点
        //   继续指向已经被 free 掉的旧节点（use-after-free）。
        node->children[index] = deleteKey(child, key);
    }

    /* 收尾处理两种情况：
     *   ① 根是叶子且被删空 → 整棵树变成空树（返回 NULL），而不是留一个
     *      「0 个关键字的空叶子」—— 那种节点不满足性质②，是个非法结构。
     *   ② 根是内部节点且被删空 → 让唯一的孩子当新根，树高降低一层。
     * 非根节点不会走到这里：下行前保证了子节点至少有 MIN_KEYS+1 个关键字，
     * 所以它自己即便合并掉一个，也至少还剩 1 个。 */
    if (node->num_keys == 0) {
        if (node->is_leaf) {
            free(node);
            return NULL;
        }
        BTreeNode *newRoot = node->children[0];
        free(node);
        node = newRoot;
    }

    return node;
}

/* ============================ 5. 遍历与打印 ============================ */

/*
 * 中序遍历：先递归第 i 个孩子，再打印第 i 个关键字，最后处理最后一个孩子。
 * B 树的关键字天然有序，所以中序遍历的输出必然是升序 —— 校验排序性最直观的手段。
 */
void inorderTraversal(BTreeNode *node) {
    if (!node) return;
    for (int i = 0; i < node->num_keys; i++) {
        inorderTraversal(node->children[i]);
        printf("%d ", node->keys[i]);
    }
    inorderTraversal(node->children[node->num_keys]);
}

/*
 * 按层打印（每个节点一行，标出层号）。
 * 对 B 树来说「层」的视角最有用：一眼就能看出树有多矮、每层有几个节点。
 */
void printTree(BTreeNode *node, int level) {
    if (!node) return;
    printf("level %d [%d keys]: ", level, node->num_keys);
    for (int i = 0; i < node->num_keys; i++) {
        printf("%d ", node->keys[i]);
    }
    printf("\n");
    for (int i = 0; i <= node->num_keys; i++) {
        printTree(node->children[i], level + 1);
    }
}

/* ============================ 6. 校验与释放 ============================ */

/*
 * 递归校验一棵子树是否满足 B 树性质。参数说明：
 *   is_root        是否是整棵树的根（根的关键字下限更宽松）
 *   lower, upper   子树里所有关键字必须严格落在开区间 (lower, upper) 内
 *   depth          当前深度，用来检查「所有叶子在同一层」
 *   leaf_depth     记录遇到的第一片叶子的深度，之后的叶子必须与它相同
 *
 * ★ 这是排查 B 树实现错误最有效的工具：
 *   分裂 / 合并 / 借位只要有一处写错，关键字个数、区间或叶子层高必然出问题；
 *   每次操作后校验一次，错误就会当场暴露，而不是等到最后莫名崩溃。
 */
static bool validateSubtree(BTreeNode *node, bool is_root, int lower, int upper,
                            int depth, int *leaf_depth) {
    if (node == NULL) return true;

    bool ok = true;

    /* 性质①②③：关键字个数必须在允许范围内 */
    int min_allowed = is_root ? 1 : MIN_KEYS;
    if (node->num_keys < min_allowed || node->num_keys > MAX_KEYS) {
        printf("  [x] key count %d out of range [%d, %d] at depth %d\n",
               node->num_keys, min_allowed, MAX_KEYS, depth);
        ok = false;
    }

    /* 性质④：节点内严格升序，且都落在限定区间内 */
    for (int i = 0; i < node->num_keys; i++) {
        if (i > 0 && node->keys[i - 1] >= node->keys[i]) {
            printf("  [x] keys inside a node are not strictly increasing\n");
            ok = false;
        }
        if (node->keys[i] <= lower || node->keys[i] >= upper) {
            printf("  [x] key %d outside its subtree range (%d, %d)\n",
                   node->keys[i], lower, upper);
            ok = false;
        }
    }

    /* 性质⑤：所有叶子必须在同一层 */
    if (node->is_leaf) {
        if (*leaf_depth == -1) {
            *leaf_depth = depth;                 /* 记下第一片叶子的深度 */
        } else if (*leaf_depth != depth) {
            printf("  [x] leaves not on the same level: depth %d vs %d\n",
                   depth, *leaf_depth);
            ok = false;
        }
        return ok;
    }

    /* 内部节点：孩子个数必须等于关键字数 + 1，且每个孩子都要递归检查 */
    for (int i = 0; i <= node->num_keys; i++) {
        if (node->children[i] == NULL) {
            printf("  [x] internal node child %d is NULL (expected %d children)\n",
                   i, node->num_keys + 1);
            ok = false;
            continue;
        }
        int child_lower = (i == 0) ? lower : node->keys[i - 1];
        int child_upper = (i == node->num_keys) ? upper : node->keys[i];

        if (!validateSubtree(node->children[i], false, child_lower, child_upper,
                             depth + 1, leaf_depth)) {
            ok = false;
        }
    }
    return ok;
}

/* 对外入口：整棵树是不是一棵合法的 B 树 */
bool validateTree(BTreeNode *root) {
    if (root == NULL) return true;              /* 空树视为合法 */
    int leaf_depth = -1;
    return validateSubtree(root, true, INT_MIN, INT_MAX, 0, &leaf_depth);
}

/* 统计整棵树的关键字总数（用来检查「删一个键，总数是否恰好减 1」） */
int countKeys(BTreeNode *node) {
    if (!node) return 0;

    int total = node->num_keys;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            total += countKeys(node->children[i]);
        }
    }
    return total;
}

/* 销毁整棵树：先递归释放所有孩子，最后释放自己 */
void destroyTree(BTreeNode *node) {
    if (!node) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroyTree(node->children[i]);
        }
    }
    free(node);
}

/* ============================ 7. main 演示 ============================ */

/* 中序结果收集 + 检查严格升序 */
static int collectInorder(BTreeNode *node, int *out, int idx) {
    if (!node) return idx;
    for (int i = 0; i < node->num_keys; i++) {
        idx = collectInorder(node->children[i], out, idx);
        out[idx++] = node->keys[i];
    }
    return collectInorder(node->children[node->num_keys], out, idx);
}

static bool inorderSorted(BTreeNode *root, int count) {
    static int buf[4096];
    if (count > (int)(sizeof(buf) / sizeof(buf[0]))) return false;
    if (collectInorder(root, buf, 0) != count) return false;
    for (int i = 1; i < count; i++) {
        if (buf[i - 1] >= buf[i]) return false;
    }
    return true;
}

/* 小型伪随机数发生器：固定种子 → 每次运行结果一致，便于对比和排查 */
static unsigned int g_seed = 20260918u;

static unsigned int nextRand(void) {
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}

/* 洗牌（Fisher-Yates） */
static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(nextRand() % (unsigned int)(i + 1));
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/*
 * 树高（层数）。B 树是完美平衡的：所有叶子同层，
 * 所以任意一条「根 → 叶子」的路径都一样长，顺着 children[0] 走到底即可。
 */
static int treeHeight(BTreeNode *node) {
    if (!node) return 0;
    return 1 + treeHeight(node->children[0]);
}

int main(void) {
    printf("==========================================================\n");
    printf("  B-tree demo (order M = %d, max %d keys, min %d keys)\n",
           M, MAX_KEYS, MIN_KEYS);
    printf("==========================================================\n\n");

    BTreeNode *root = NULL;

    /* ---------------- [1] 插入：观察分裂与树长高 ---------------- */
    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17, 3, 25};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    printf("[1] Insert: ");
    for (int i = 0; i < n; i++) printf("%d ", keys[i]);
    printf("\n");
    printf("    A node holds %d keys, so the %dth key pushed into one node\n",
           MAX_KEYS, MAX_KEYS + 1);
    printf("    forces a split and the tree grows one level.\n\n");

    for (int i = 0; i < n; i++) {
        root = insert(root, keys[i]);
        printf("---- after inserting %d ----\n", keys[i]);
        printTree(root, 1);
        printf("height: %d    keys: %d    valid B-tree: %s\n",
               treeHeight(root), countKeys(root), validateTree(root) ? "PASS" : "FAIL");
        printf("inorder: "); inorderTraversal(root); printf("\n\n");
    }

    /* ---------------- [2] 查找 ---------------- */
    printf("[2] Search\n");
    int searchKeys[] = {6, 15, 30, 99};
    for (int i = 0; i < 4; i++) {
        int pos = -1;
        if (search(root, searchKeys[i], &pos)) {
            printf("  %3d -> found at position %d inside its node\n", searchKeys[i], pos);
        } else {
            printf("  %3d -> not found\n", searchKeys[i]);
        }
    }
    printf("\n");

    /* ---------------- [3] 删除：借位、合并、树高降低都会被触发 ---------------- */
    printf("[3] Delete demo\n");
    printf("    Keys are removed one by one; watch the tree borrow from a\n");
    printf("    sibling, merge nodes, and finally shrink its height.\n\n");

    int deleteKeys[] = {3, 5, 6, 7, 10, 12, 17, 20, 25, 30, 99};
    int delCount = (int)(sizeof(deleteKeys) / sizeof(deleteKeys[0]));

    for (int i = 0; i < delCount; i++) {
        int pos = -1;
        bool exists = search(root, deleteKeys[i], &pos);
        int before = countKeys(root);

        printf("---- delete %d : %s ----\n", deleteKeys[i],
               exists ? "found, removing" : "not in the tree, nothing changes");

        root = deleteKey(root, deleteKeys[i]);

        int after = countKeys(root);
        int expected = exists ? before - 1 : before;
        if (after != expected) {
            printf("  [x] key count is %d but should be %d\n", after, expected);
        }

        if (root == NULL) {
            printf("(empty tree)\n");
        } else {
            printTree(root, 1);
        }
        printf("height: %d    keys: %d    valid B-tree: %s\n",
               treeHeight(root), countKeys(root), validateTree(root) ? "PASS" : "FAIL");
        printf("inorder: "); inorderTraversal(root); printf("\n\n");
    }

    destroyTree(root);
    root = NULL;

    /* ---------------- [4] 随机压力测试 ---------------- */
    printf("[4] Stress test: 500 random inserts + 500 random deletes\n");
    printf("    After every operation we check all 5 B-tree properties\n");
    printf("    plus the sorted order of the inorder sequence.\n\n");

    enum { STRESS_N = 500 };
    static int data[STRESS_N];
    for (int i = 0; i < STRESS_N; i++) data[i] = i + 1;

    BTreeNode *big = NULL;
    int steps = 0, pass = 0, fail = 0;
    bool sortedOK = true;

    /* 第一段：把 1..500 打乱顺序插进去 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        big = insert(big, data[i]);
        steps++;
        if (validateTree(big)) pass++; else fail++;
        if (!inorderSorted(big, countKeys(big))) sortedOK = false;
    }
    printf("    after %d inserts : keys = %d, tree height = %d\n",
           STRESS_N, countKeys(big), treeHeight(big));
    printf("    (that is why B-trees are used for disk indexes:\n");
    printf("     %d keys fit in just %d levels, so a lookup needs very few reads)\n\n",
           STRESS_N, treeHeight(big));

    /* 第二段：再打乱一次顺序，把所有键逐个删光 */
    shuffle(data, STRESS_N);
    for (int i = 0; i < STRESS_N; i++) {
        int before = countKeys(big);
        big = deleteKey(big, data[i]);
        if (countKeys(big) != before - 1) {
            printf("    ERROR: deleting %d did not remove exactly one key\n", data[i]);
            fail++;
        }
        steps++;
        if (validateTree(big)) pass++; else fail++;
        if (!inorderSorted(big, countKeys(big))) sortedOK = false;
    }

    /* 再删几个不存在的键，确认不会误删、也不会破坏结构 */
    for (int k = 9000; k < 9005; k++) {
        big = deleteKey(big, k);
        if (validateTree(big)) pass++; else fail++;
    }

    printf("    operations validated : %d\n", steps);
    printf("    property check PASS  : %d\n", pass);
    printf("    property check FAIL  : %d\n", fail);
    printf("    inorder always sorted: %s\n", sortedOK ? "YES" : "NO");
    printf("    remaining keys       : %d\n\n", countKeys(big));

    /* ---------------- [5] 释放 ---------------- */
    destroyTree(big);
    printf("[5] All trees have been freed\n");

    return 0;
}
