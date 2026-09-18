/*
 * ============================================================================
 *  哈夫曼树（Huffman Tree）与哈夫曼编码 完整演示
 * ============================================================================
 *
 *  【一、要解决什么问题】
 *  给一段文本编码成 01 串。如果每个字符都用等长的 8 位，很浪费：
 *  出现 100 次的和出现 1 次的字符花一样多的比特。
 *  哈夫曼的想法很朴素：★ 出现越多的字符，编码越短 ★。
 *
 *  【二、怎么构造（贪心）】
 *  把每个字符看成一个带权（权 = 出现次数）的孤立节点，然后：
 *        ① 从森林里挑出权值最小的两个节点，合并成一棵新树，新根权值 = 两者之和
 *        ② 把新树放回森林
 *        ③ 重复 ①②，直到森林里只剩一棵树 —— 它就是哈夫曼树
 *  权值大的节点自然被「抬」得离根更近，路径更短。
 *
 *  【三、为什么它能解码（前缀码）】
 *  字符全部放在**叶子**上，于是任何编码都不可能是另一个编码的前缀：
 *        有 '0'、'01' 两个码时，读到 01 就不知道是「0 后面跟 1」还是「01」—— 歧义
 *  叶子保证不会发生这种情况。解码时从根出发，按位往左（0）/往右（1）走，
 *  走到叶子就吐出一个字符，然后回到根继续。
 *  本文件会**用程序验证前缀性**，而不是嘴上说说。
 *
 *  【四、两个容易搞错的点】
 *        - 哈夫曼树**不唯一**：权值相同的节点谁左谁右、谁先谁后都可以，
 *          但「带权路径长度 WPL」一定是唯一的（都是最优值）
 *        - WPL = Σ(叶子权值 × 该叶子的深度) = 所有内部节点（含根）权值之和，
 *          两种算法结果必须相同 —— 本文件两种都算，互相印证
 *
 *  【五、文件结构】
 *    1. 类型定义与建节点   Node / createNode    ← 保留原始代码，只补注释
 *    2. 构造               buildHuffman（挑选最小两个 + 合并）
 *    3. 统计               wpl、weightedPathSum、averageCodeLength
 *    4. 编码与解码         genCodes / encodeText / decodeText
 *    5. 打印与释放         printTree / destroyTree
 *    6. 自检工具           expectInt / expectStr
 *    7. main 演示（用《数据结构》教材的经典权值，WPL = 271）
 *
 *  约定：输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_SYM  64      /* 最多多少个待编码的字符 */
#define MAX_CODE 64      /* 单个字符编码的最大长度（01 串） */

/* ============ 1. 类型定义与建节点（保留原始代码，只补注释）============ */

/*
 * 哈夫曼树的节点。
 *   data      —— 字符（**只有叶子才有意义**，内部节点用 '\0' 占位）
 *   frequency —— 权值（出现次数）。内部节点的权值 = 两个孩子权值之和
 * 注意它和 BST 的节点长得一样（data/left/right），但没有任何「左小右大」的约束：
 * 这棵树不是搜索树，它的用途只有一个 —— 让高频字符靠近根。
 */
typedef struct Node {
    char data;
    int frequency;
    struct Node *left;
    struct Node *right;
} Node;

Node *createNode(char data, int frequency) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->frequency = frequency;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------ */
Node *buildHuffman(const char *chars, const int *freq, int n);
int   wpl(Node *root, int depth);
int   weightSum(Node *root);
int   countLeaves(Node *root);
void  genCodes(Node *node, char *buf, int depth, char *table, int *n);
int   encodeText(const char *text, const char *table, int tableN, char *out);
bool  decodeText(Node *root, const char *bits, char *out);
void  printTree(Node *node, int depth, char side);
void  destroyTree(Node *root);

/* ======================= 2. 构造哈夫曼树 ======================= */

/*
 * 由「字符 + 权值」构造哈夫曼树，返回根节点。
 *
 * 森林用一个数组表示（只存还活着的树根）。每一轮：
 *        ① 线性扫描找出权值最小的两个下标 a、b
 *        ② 造一个新根（字符为 '\0'，权值为两者之和），把 a、b 挂上去
 *        ③ 从森林里删掉 a、b，把新树放进去
 * 重复到只剩一棵树。
 *
 * 复杂度 O(n²)（每次扫描 O(n)）。n 很大时应该用最小堆，把选最小的两个降到 O(log n)，
 * 但本章重点是「贪心思想」，用数组更直观。
 */
Node *buildHuffman(const char *chars, const int *freq, int n) {
    if (n <= 0) return NULL;

    Node *forest[MAX_SYM];
    int cnt = 0;

    for (int i = 0; i < n; i++) {
        forest[cnt++] = createNode(chars[i], freq[i]);   /* 一开始每个字符是一棵单节点树 */
    }

    while (cnt > 1) {
        /* ① 找最小（a）与次小（b）：一次扫描同时维护两个最小值 */
        int a = 0, b = 1;
        if (forest[b]->frequency < forest[a]->frequency) {
            int t = a; a = b; b = t;
        }
        for (int i = 2; i < cnt; i++) {
            if (forest[i]->frequency < forest[a]->frequency) {
                b = a; a = i;
            } else if (forest[i]->frequency < forest[b]->frequency) {
                b = i;
            }
        }

        /* ② 合并 */
        Node *parent = createNode('\0', forest[a]->frequency + forest[b]->frequency);
        parent->left  = forest[a];
        parent->right = forest[b];

        /* ③ 从森林里摘掉 a、b（用「末尾元素搬过来」的 O(1) 删除法），再放入新树。
         *    先删下标大的，避免搬动影响另一个下标。 */
        int hi = (a > b) ? a : b;
        int lo = (a < b) ? a : b;
        forest[hi] = forest[--cnt];
        forest[lo] = forest[--cnt];
        forest[cnt++] = parent;
    }

    return forest[0];
}

/* ======================= 3. 统计 ======================= */

/* 带权路径长度：叶子权值 × 叶子深度，累加（这是哈夫曼树的核心指标） */
int wpl(Node *root, int depth) {
    if (root == NULL) return 0;
    if (root->left == NULL && root->right == NULL) {
        return root->frequency * depth;          /* 叶子：算它一份 */
    }
    return wpl(root->left, depth + 1) + wpl(root->right, depth + 1);
}

/* 另一种等价算法：把所有内部节点（含根）的权值加起来，结果必须与 wpl 相同 */
int weightSum(Node *root) {
    if (root == NULL || (root->left == NULL && root->right == NULL)) return 0;
    return root->frequency + weightSum(root->left) + weightSum(root->right);
}

/* 叶子数 = 有多少个字符拿到了编码 */
int countLeaves(Node *root) {
    if (root == NULL) return 0;
    if (root->left == NULL && root->right == NULL) return 1;
    return countLeaves(root->left) + countLeaves(root->right);
}

/* ======================= 4. 编码与解码 ======================= */

/*
 * 生成编码表：从根往下走，往左记 '0'，往右记 '1'，走到叶子就得到该字符的编码。
 * 这里用递归 + 一个「路径缓冲区」buf，depth 表示当前写到第几位。
 * ★ 只有叶子才对应一个字符 —— 这正是「前缀码」的由来。
 */
void genCodes(Node *node, char *buf, int depth, char *table, int *n) {
    if (node == NULL) return;

    if (node->left == NULL && node->right == NULL) {
        buf[depth] = '\0';
        /* 把字符和它的编码成对存进 table：table[0]=字符, table[1..]=编码 */
        table[(*n) * MAX_CODE] = node->data;
        strcpy(&table[(*n) * MAX_CODE + 1], buf);
        (*n)++;
        return;
    }

    buf[depth] = '0';
    genCodes(node->left, buf, depth + 1, table, n);
    buf[depth] = '1';
    genCodes(node->right, buf, depth + 1, table, n);
}

/* 查表找一个字符的编码，找不到返回 NULL */
static const char *codeOf(char c, const char *table, int tableN) {
    for (int i = 0; i < tableN; i++) {
        if (table[i * MAX_CODE] == c) {
            return &table[i * MAX_CODE + 1];
        }
    }
    return NULL;
}

/* 把文本编码成 01 串；遇到表里没有的字符返回 -1 */
int encodeText(const char *text, const char *table, int tableN, char *out) {
    int len = 0;

    for (const char *p = text; *p != '\0'; p++) {
        const char *code = codeOf(*p, table, tableN);
        if (code == NULL) return -1;
        for (const char *q = code; *q != '\0'; q++) {
            out[len++] = *q;
        }
    }
    out[len] = '\0';
    return len;
}

/*
 * 解码：从根开始，每位 0 走左、1 走右；到叶子就输出字符并回到根。
 * 返回 true 表示「解码完整」：既没走错路，最后也正好停在根上
 * （停在别处说明最后一段编码是残缺的，属于非法输入）。
 */
bool decodeText(Node *root, const char *bits, char *out) {
    if (root == NULL) return false;

    Node *cur = root;
    int n = 0;

    for (const char *p = bits; *p != '\0'; p++) {
        if (*p == '0')      cur = cur->left;
        else if (*p == '1') cur = cur->right;
        else return false;                      /* 非法字符 */

        if (cur == NULL) return false;          /* 走到空处：编码不合法 */

        if (cur->left == NULL && cur->right == NULL) {
            out[n++] = cur->data;               /* 到叶子，吐出一个字符 */
            cur = root;                         /* 回到根，准备下一个 */
        }
    }
    out[n] = '\0';

    return (cur == root);                       /* 必须正好停在根上 */
}

/* ======================= 5. 打印与释放 ======================= */

/*
 * 缩进打印：内部节点没有字符，用 [*] 表示，括号里是权值。
 * 能看到「权值大的靠近根、权值小的在深处」—— 这正是哈夫曼树的样子。
 */
void printTree(Node *node, int depth, char side) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("    ");
    if (side == 'L')      printf("L ");
    else if (side == 'R') printf("R ");

    if (node->data == '\0') printf("[*] w=%d\n", node->frequency);
    else                    printf("%c w=%d\n", node->data, node->frequency);

    printTree(node->left,  depth + 1, 'L');
    printTree(node->right, depth + 1, 'R');
}

/* 递归释放：先孩子后自己 */
void destroyTree(Node *root) {
    if (root == NULL) return;
    destroyTree(root->left);
    destroyTree(root->right);
    free(root);
}

/* ======================= 6. 自检工具 ======================= */

static int g_pass = 0;
static int g_fail = 0;

static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-32s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-32s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

static void expectStr(const char *what, const char *got, const char *want) {
    if (strcmp(got, want) == 0) {
        printf("  [ok]   %-32s = \"%s\"\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-32s = \"%s\" (expected \"%s\")\n", what, got, want);
        g_fail++;
    }
}

/*
 * 前缀性检查：任意两个编码，谁也不能是对方的前缀。
 * 这是「能唯一解码」的充要条件，也是哈夫曼编码最该被验证的性质。
 */
static bool isPrefixFree(const char *table, int tableN) {
    for (int i = 0; i < tableN; i++) {
        const char *ci = &table[i * MAX_CODE + 1];
        for (int j = 0; j < tableN; j++) {
            if (i == j) continue;
            const char *cj = &table[j * MAX_CODE + 1];
            if (strncmp(ci, cj, strlen(ci)) == 0) return false;   /* ci 是 cj 的前缀 */
        }
    }
    return true;
}

/* ============================ 7. main 演示 ============================ */

/*
 * 用《数据结构》教材里的经典权值：3 5 7 8 11 14 23 29（总和正好 100）。
 * 不管权值相同的节点怎么摆，最优 WPL 都是 271 —— 这个数字可以拿来当自检的预期值。
 * 给它们配上字符 A..H，方便演示编码与解码。
 */
int main(void) {
    printf("==========================================================\n");
    printf("  Huffman tree / Huffman coding demo\n");
    printf("==========================================================\n\n");

    static const char chars[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    static const int  freq[]  = { 5,  29,   7,   8,  14,  23,   3,  11 };
    int n = (int)(sizeof(freq) / sizeof(freq[0]));

    int total = 0;
    for (int i = 0; i < n; i++) total += freq[i];

    printf("[1] Symbols and frequencies\n  ");
    for (int i = 0; i < n; i++) printf("%c:%d  ", chars[i], freq[i]);
    printf("\n  total = %d\n\n", total);

    /* ---------------- [2] 建树 ---------------- */
    Node *root = buildHuffman(chars, freq, n);

    printf("[2] The Huffman tree (leaf = symbol, [*] = internal node)\n");
    printTree(root, 0, ' ');
    printf("\n");

    /* ---------------- [3] 编码表 ---------------- */
    static char table[MAX_SYM * MAX_CODE];
    int tableN = 0;
    char buf[MAX_CODE];
    genCodes(root, buf, 0, table, &tableN);

    printf("[3] Code table\n");
    for (int i = 0; i < tableN; i++) {
        char ch = table[i * MAX_CODE];
        int w = 0;
        for (int k = 0; k < n; k++) {          /* 按字符反查权值（表的顺序是树的顺序） */
            if (chars[k] == ch) { w = freq[k]; break; }
        }
        printf("  '%c' (w=%2d) -> %s\n", ch, w, &table[i * MAX_CODE + 1]);
    }
    printf("\n");

    /* ---------------- [4] 统计 ---------------- */
    int w1 = wpl(root, 0);
    int w2 = weightSum(root);

    printf("[4] Statistics\n");
    printf("  WPL (sum of leaf freq x depth) = %d\n", w1);
    printf("  WPL (sum of internal weights)  = %d   <- must be the same\n", w2);
    printf("  average code length            = %.2f bits/char\n", (double)w1 / total);
    printf("  (fixed 8-bit coding would need 8.00 bits/char)\n\n");

    /* ---------------- [5] 编码 / 解码 往返 ---------------- */
    static const char text[] = "BADGE";
    static char bits[512];
    static char back[512];

    int bitLen = encodeText(text, table, tableN, bits);
    bool ok = decodeText(root, bits, back);

    printf("[5] Round trip\n");
    printf("  text      : %s\n", text);
    printf("  encoded   : %s  (%d bits, %.2f bits/char average here)\n",
           bits, bitLen, bitLen > 0 ? (double)bitLen / (double)strlen(text) : 0.0);
    printf("  decoded   : %s\n\n", ok ? back : "(decode failed)");

    /* ---------------- [6] 自检 ---------------- */
    printf("[6] Self checks\n");
    expectInt("WPL",                        w1, 271);
    expectInt("WPL via internal weights",   w2, 271);
    expectInt("number of codes",            tableN, 8);
    expectInt("prefix-free (no code is a prefix of another)",
              isPrefixFree(table, tableN) ? 1 : 0, 1);
    expectStr("encode/decode round trip",   ok ? back : "", text);
    expectInt("leaf count equals symbol count", countLeaves(root), 8);
    expectInt("average code length beats fixed 8-bit",
              ((double)w1 / total < 8.0) ? 1 : 0, 1);

    /* 顺带验证：用一个「不完整的编码」解码应当失败 */
    bool bad = decodeText(root, "0", back);
    expectInt("decoding an incomplete bit string fails", bad ? 0 : 1, 1);
    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [7] 释放 ---------------- */
    destroyTree(root);
    printf("[7] The Huffman tree has been freed\n");

    return 0;
}
