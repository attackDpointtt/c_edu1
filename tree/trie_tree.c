/*
 * ============================================================================
 *  字典树 / 前缀树（Trie）完整演示
 * ============================================================================
 *
 *  【一、Trie 的核心思想】
 *  不用节点存字符串，而是用「从根出发的一条路径」表示一个字符串：
 *  每个节点对应一个字符，走到某个节点就拼出了某个前缀。
 *
 *        插入 cat、car、card、dog 之后的形状（* 表示这里是一个完整单词）：
 *
 *        root ─ c ─ a ─ t *
 *                    └ r * ─ d *
 *               d ─ o * ─ ...
 *
 *  于是：查一个长度为 m 的单词，只看 m 个字符 —— O(m)，
 *  **和词典里有多少个词毫无关系**（对比：BST 要 O(log n) 次字符串比较）。
 *
 *  【二、代价】
 *  每个节点都要留 26 个孩子指针（int 下标是 26 个，指针数组就是 26×8 = 208 字节）。
 *  词少的时候非常浪费 —— 所以工程上会用：
 *        - 哈希表 / 有序数组代替 26 个定长槽位
 *        - 压缩 Trie（把只有一个孩子的链合并成一条边）
 *  本文件用最朴素的 26 叉写法，先把原理看清。
 *
 *  【三、Trie 能干什么】
 *        ✅ 前缀查询：输入 "car" 就能列出 car / card / care …
 *        ✅ 自动补全、拼写提示、IP 路由（二进制 Trie）、敏感词过滤
 *        ❌ 不适合「模糊匹配」「任意子串搜索」
 *
 *  【四、一个容易踩的坑】
 *  is_end_of_word 是必需的：光有路径只能说明「这个前缀存在」，
 *  不能说明「这是个词」。比如插入了 "card"，那么前缀 "car" 也存在，
 *  但 "car" 本身可能并不是一个词 —— 两件事必须分开记。
 *
 *  【五、文件结构】
 *    1. 类型定义与建节点   TrieNode / createNode   ← 保留原始代码，只补注释
 *    2. 插入与查询         insert / search / startsWith
 *    3. 统计与遍历         countWords / countNodes / printTrie / collectWords
 *    4. 删除               removeWord（顺手剪掉没用的分支）
 *    5. 释放               destroyTree
 *    6. 自检工具           expectInt / expectStr / expectSeq
 *    7. main 演示
 *
 *  约定：本文件只处理小写字母 a-z（下标 0-25）。
 *        输出提示一律英文（避免终端编码乱码），注释一律中文。
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALPHABET_SIZE 26

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    int is_end_of_word;
} TrieNode;

/* 造一个空节点：26 个孩子全空，并且它自己不是一个单词的结尾 */
TrieNode *createNode() {
    TrieNode *newNode = (TrieNode *)malloc(sizeof(TrieNode));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->is_end_of_word = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        newNode->children[i] = NULL;
    }
    return newNode;
}

/* ------------------------------ 前向声明 ------------------------------ */
void       insert(TrieNode *root, const char *word);
bool       search(TrieNode *root, const char *word);
TrieNode  *findPrefixNode(TrieNode *root, const char *prefix);
bool       startsWith(TrieNode *root, const char *prefix);
int        countWords(TrieNode *root);
int        countNodes(TrieNode *root);
void       printTrie(TrieNode *root);
int        collectWords(TrieNode *root, const char *prefix, char out[][64]);
bool       removeWord(TrieNode *root, const char *word);
void       destroyTree(TrieNode *root);

/* ======================= 1. 插入与查询 ======================= */

/* 小写字母转下标；非法字符返回 -1 */
static int charIndex(char c) {
    if (c < 'a' || c > 'z') return -1;
    return c - 'a';
}

/*
 * 插入一个单词：沿着字符一路往下走，缺节点就造一个，
 * 最后把落点的 is_end_of_word 置 1。
 * 若这个词已经存在，什么都不改变（幂等）。
 */
void insert(TrieNode *root, const char *word) {
    TrieNode *cur = root;

    for (const char *p = word; *p != '\0'; p++) {
        int idx = charIndex(*p);
        if (idx < 0) return;                 /* 非法字符，直接放弃 */

        if (cur->children[idx] == NULL) {
            cur->children[idx] = createNode();   /* 这条边还不存在 → 造节点 */
        }
        cur = cur->children[idx];
    }
    cur->is_end_of_word = 1;                 /* ★ 别忘了标记「这是个词」 */
}

/*
 * 精确查询：整条路径都存在 **并且** 终点被标记为单词结尾。
 */
bool search(TrieNode *root, const char *word) {
    TrieNode *cur = root;

    for (const char *p = word; *p != '\0'; p++) {
        int idx = charIndex(*p);
        if (idx < 0) return false;
        if (cur->children[idx] == NULL) return false;   /* 半路断了 → 不存在 */
        cur = cur->children[idx];
    }
    return cur->is_end_of_word != 0;
}

/* 走到某个前缀对应的节点；走不到就返回 NULL */
TrieNode *findPrefixNode(TrieNode *root, const char *prefix) {
    TrieNode *cur = root;

    for (const char *p = prefix; *p != '\0'; p++) {
        int idx = charIndex(*p);
        if (idx < 0) return NULL;
        if (cur->children[idx] == NULL) return NULL;
        cur = cur->children[idx];
    }
    return cur;
}

/*
 * 前缀查询：只要路径走得通就为真，**不要求**终点是个完整的词。
 * 「car」是前缀」和「car 是个词」是两件事，这是 Trie 最常用也最容易混的地方。
 */
bool startsWith(TrieNode *root, const char *prefix) {
    return findPrefixNode(root, prefix) != NULL;
}

/* ======================= 2. 统计与遍历 ======================= */

/* 这棵子树里一共有多少个单词（含终点自己在内） */
int countWords(TrieNode *root) {
    if (root == NULL) return 0;

    int total = root->is_end_of_word ? 1 : 0;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        total += countWords(root->children[i]);
    }
    return total;
}

/* 节点总数（用来看空间开销，也用来验证删除时有没有剪掉无用分支） */
int countNodes(TrieNode *root) {
    if (root == NULL) return 0;

    int total = 1;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        total += countNodes(root->children[i]);
    }
    return total;
}

/* 打印整棵 Trie：每行就是「从根走到这里的字符串」，结尾带 * 的是完整单词 */
static void printSub(TrieNode *node, char *path, int depth) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] == NULL) continue;

        path[depth] = (char)('a' + i);
        path[depth + 1] = '\0';
        printf("  %*s%c%s\n", depth * 2, "", (char)('a' + i),
               node->children[i]->is_end_of_word ? "  *" : "");
        printSub(node->children[i], path, depth + 1);
    }
}

void printTrie(TrieNode *root) {
    char path[64];
    printSub(root, path, 0);
}

/*
 * 收集「以 prefix 开头」的所有单词。
 * 做法：先走到前缀终点，再从那里往下收集（这样就不用重复处理前缀部分了）。
 * 返回收集到的单词个数，结果写进 out（每行最多 64 字节）。
 */
static void collectSub(TrieNode *node, char *path, int depth, char out[][64], int *n) {
    if (node->is_end_of_word) {
        path[depth] = '\0';
        strcpy(out[*n], path);
        (*n)++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) {
            path[depth] = (char)('a' + i);
            collectSub(node->children[i], path, depth + 1, out, n);
        }
    }
}

int collectWords(TrieNode *root, const char *prefix, char out[][64]) {
    TrieNode *start = findPrefixNode(root, prefix);

    int n = 0;
    if (start == NULL) return 0;

    char path[64];
    int len = (int)strlen(prefix);
    strcpy(path, prefix);
    collectSub(start, path, len, out, &n);
    return n;
}

/* ======================= 3. 删除 ======================= */

/* 这个节点还有孩子吗？ */
static bool hasChildren(TrieNode *node) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) return true;
    }
    return false;
}

/*
 * 删除单词（递归实现）。
 * 返回值表示：**这个节点在删除后是否已经没用了**（不是单词结尾、也没有孩子）。
 * 上层收到 true 就把这个孩子 free 掉 —— 这就是「顺手剪枝」，
 * 否则字典树会越删越肿（全是空壳节点）。
 * 注意：节点自己不能在这里 free，因为要由父亲把它的指针置空。
 */
static bool removeRec(TrieNode *node, const char *word, int depth) {
    if (node == NULL) return false;

    if (word[depth] == '\0') {
        if (!node->is_end_of_word) return false;   /* 这个词本来就不在树上 */
        node->is_end_of_word = 0;                  /* ① 取消「单词结尾」标记 */
        return !hasChildren(node);                 /* ② 没孩子了就告诉父亲：可以删我 */
    }

    int idx = charIndex(word[depth]);
    if (idx < 0) return false;

    if (removeRec(node->children[idx], word, depth + 1)) {
        free(node->children[idx]);                 /* 孩子没用了 → 释放 */
        node->children[idx] = NULL;
        return (!node->is_end_of_word && !hasChildren(node));
    }
    return false;
}

/* 对外接口：删除一个单词。根节点永远保留（空的 Trie 也是一棵合法的 Trie）。 */
bool removeWord(TrieNode *root, const char *word) {
    if (!search(root, word)) return false;         /* 先确认它真的在，避免误剪分支 */
    (void)removeRec(root, word, 0);
    return true;
}

/* ======================= 4. 释放 ======================= */

/*
 * 递归释放整棵树：先把 26 个孩子都释放掉，最后释放自己。
 * 注意 Trie 的每个节点都有 26 个指针，要逐个检查。
 */
void destroyTree(TrieNode *root) {
    if (root == NULL) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        destroyTree(root->children[i]);
    }
    free(root);
}

/* ======================= 5. 自检工具 ======================= */

static int g_pass = 0;
static int g_fail = 0;

static void expectInt(const char *what, int got, int want) {
    if (got == want) {
        printf("  [ok]   %-34s = %d\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-34s = %d (expected %d)\n", what, got, want);
        g_fail++;
    }
}

static void expectBool(const char *what, bool got, bool want) {
    printf("  [%s] %-34s = %s\n", got == want ? "ok  " : "FAIL",
           what, got ? "yes" : "no");
    if (got == want) g_pass++; else g_fail++;
}

static void expectStr(const char *what, const char *got, const char *want) {
    if (strcmp(got, want) == 0) {
        printf("  [ok]   %-34s = %s\n", what, got);
        g_pass++;
    } else {
        printf("  [FAIL] %-34s = %s (expected %s)\n", what, got, want);
        g_fail++;
    }
}

/* 把字符串数组做一次插入排序，保证打印与比对的顺序稳定 */
static void sortStrings(char arr[][64], int n) {
    for (int i = 1; i < n; i++) {
        char key[64];
        strcpy(key, arr[i]);
        int j = i - 1;
        while (j >= 0 && strcmp(arr[j], key) > 0) {
            strcpy(arr[j + 1], arr[j]);
            j--;
        }
        strcpy(arr[j + 1], key);
    }
}

/* ============================ 6. main 演示 ============================ */

int main(void) {
    printf("==========================================================\n");
    printf("  Trie (prefix tree) demo\n");
    printf("==========================================================\n\n");

    TrieNode *root = createNode();

    static const char *words[] = {"cat", "car", "card", "care", "dog", "do", "done"};
    int wn = (int)(sizeof(words) / sizeof(words[0]));

    /* ---------------- [1] 插入 ---------------- */
    printf("[1] Insert:");
    for (int i = 0; i < wn; i++) {
        printf(" %s", words[i]);
        insert(root, words[i]);
    }
    printf("\n\n");

    printf("[2] The trie (* marks a complete word)\n");
    printTrie(root);
    printf("\n");

    /* ---------------- [3] 查询 ---------------- */
    printf("[3] Queries\n");
    printf("  search(\"cat\")  = %s\n", search(root, "cat")  ? "yes" : "no");
    printf("  search(\"ca\")   = %s   <- only a prefix, not a word\n", search(root, "ca") ? "yes" : "no");
    printf("  search(\"card\") = %s\n", search(root, "card") ? "yes" : "no");
    printf("  search(\"careful\") = %s\n", search(root, "careful") ? "yes" : "no");
    printf("  startsWith(\"car\") = %s\n", startsWith(root, "car") ? "yes" : "no");
    printf("  startsWith(\"z\")   = %s\n\n", startsWith(root, "z") ? "yes" : "no");

    /* ---------------- [4] 前缀补全 ---------------- */
    static char found[16][64];
    int fn = collectWords(root, "car", found);
    sortStrings(found, fn);

    printf("[4] Words starting with \"car\" (%d of them)\n", fn);
    for (int i = 0; i < fn; i++) printf("  %s\n", found[i]);
    printf("\n");

    printf("[5] Size\n");
    printf("  words = %d   nodes = %d\n\n", countWords(root), countNodes(root));

    /* ---------------- [6] 删除 ---------------- */
    int nodesBefore = countNodes(root);
    bool removed = removeWord(root, "card");
    int nodesAfter = countNodes(root);

    printf("[6] Remove \"card\" (%s)\n", removed ? "done" : "not found");
    printf("  nodes: %d -> %d   (the branch was pruned)\n", nodesBefore, nodesAfter);
    printf("  search(\"card\") = %s\n", search(root, "card") ? "yes" : "no");
    printf("  search(\"car\")  = %s   <- \"car\" itself is still a word\n", search(root, "car") ? "yes" : "no");
    printf("  search(\"care\") = %s   <- its sibling is untouched\n\n", search(root, "care") ? "yes" : "no");

    /* ---------------- [7] 自检 ---------------- */
    printf("[7] Self checks\n");
    expectBool("search(\"cat\")",          search(root, "cat"),   true);
    expectBool("search(\"ca\")  [prefix]", search(root, "ca"),    false);
    expectBool("search(\"card\") [removed]", search(root, "card"), false);
    expectBool("search(\"care\")",         search(root, "care"),  true);
    expectBool("search(\"do\")",           search(root, "do"),    true);
    expectBool("search(\"done\")",         search(root, "done"),  true);
    expectBool("search(\"d\")   [prefix]", search(root, "d"),     false);
    expectBool("startsWith(\"car\")",      startsWith(root, "car"), true);
    expectBool("startsWith(\"z\")",        startsWith(root, "z"),   false);

    expectInt("words starting with \"car\"", fn, 3);
    expectStr("first such word",  found[0], "car");
    expectStr("second such word", found[1], "card");
    expectStr("third such word",  found[2], "care");
    expectInt("word count after removal", countWords(root), 6);
    expectInt("branch was pruned", nodesAfter < nodesBefore ? 1 : 0, 1);
    expectInt("removing a missing word returns false",
              removeWord(root, "zebra") ? 0 : 1, 1);

    printf("\n  result: %d passed, %d failed\n\n", g_pass, g_fail);

    /* ---------------- [8] 释放 ---------------- */
    destroyTree(root);
    printf("[8] The trie has been freed\n");

    return 0;
}
