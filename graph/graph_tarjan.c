#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 100010  // 最大节点数
#define MAXM 500010  // 最大边数

// ==================== 链式前向星存图 ====================
int head[MAXN], to[MAXM], nxt[MAXM];
int cnt = 0;

void add_edge(int u, int v) {
    cnt++;
    to[cnt] = v;
    nxt[cnt] = head[u];
    head[u] = cnt;
}

// ==================== 1. 求强连通分量（有向图） ====================
int dfn[MAXN], low[MAXN];
int scc[MAXN], scc_size[MAXN];
int dfncnt = 0, scc_cnt = 0;
int stk[MAXN], top = 0;
int in_stack[MAXN];

void tarjan_scc(int u) {
    dfn[u] = low[u] = ++dfncnt;
    stk[++top] = u;
    in_stack[u] = 1;

    for (int i = head[u]; i; i = nxt[i]) {
        int v = to[i];
        if (!dfn[v]) {
            tarjan_scc(v);
            if (low[v] < low[u]) low[u] = low[v];
        } else if (in_stack[v]) {
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }

    if (dfn[u] == low[u]) {
        scc_cnt++;
        int v;
        do {
            v = stk[top--];
            in_stack[v] = 0;
            scc[v] = scc_cnt;
            scc_size[scc_cnt]++;
        } while (v != u);
    }
}

// ==================== 2. 求割点（无向图） ====================
int cut_point[MAXN];  // 标记是否为割点

void tarjan_cut_point(int u, int parent) {
    dfn[u] = low[u] = ++dfncnt;
    int child = 0;

    for (int i = head[u]; i; i = nxt[i]) {
        int v = to[i];
        if (!dfn[v]) {
            child++;
            tarjan_cut_point(v, u);
            if (low[v] < low[u]) low[u] = low[v];

            // 割点判定条件
            if (!parent && child > 1) {  // 根节点：子树数 > 1
                cut_point[u] = 1;
            }
            if (parent && low[v] >= dfn[u]) {  // 非根节点
                cut_point[u] = 1;
            }
        } else if (v != parent) {  // 反向边，且不是父节点
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }
}

// ==================== 3. 求割边/桥（无向图） ====================
typedef struct {
    int u, v;
} Edge;

Edge bridge[MAXM];
int bridge_cnt = 0;

void tarjan_bridge(int u, int parent) {
    dfn[u] = low[u] = ++dfncnt;

    for (int i = head[u]; i; i = nxt[i]) {
        int v = to[i];
        if (!dfn[v]) {
            tarjan_bridge(v, u);
            if (low[v] < low[u]) low[u] = low[v];

            // 割边判定条件：low[v] > dfn[u]
            if (low[v] > dfn[u]) {
                bridge[++bridge_cnt].u = u;
                bridge[bridge_cnt].v = v;
            }
        } else if (v != parent) {  // 反向边
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }
}

// ==================== 初始化 ====================
void init(int n) {
    cnt = 0;
    dfncnt = 0;
    scc_cnt = 0;
    top = 0;
    bridge_cnt = 0;
    memset(head, 0, sizeof(head));
    memset(dfn, 0, sizeof(dfn));
    memset(low, 0, sizeof(low));
    memset(scc, 0, sizeof(scc));
    memset(scc_size, 0, sizeof(scc_size));
    memset(in_stack, 0, sizeof(in_stack));
    memset(cut_point, 0, sizeof(cut_point));
}

// ==================== 主函数 ====================
int main() {
    int n, m;
    printf("请输入节点数和边数: ");
    scanf("%d %d", &n, &m);

    init(n);

    printf("请输入 %d 条边 (u v):\n", m);
    for (int i = 0; i < m; i++) {
        int u, v;
        scanf("%d %d", &u, &v);
        add_edge(u, v);
        add_edge(v, u);  // 无向图需要双向加边
    }

    // 1. 求强连通分量（有向图，注释掉双向加边后使用）
    /*
    for (int i = 1; i <= n; i++) {
        if (!dfn[i]) tarjan_scc(i);
    }
    printf("\n共找到 %d 个强连通分量:\n", scc_cnt);
    for (int i = 1; i <= scc_cnt; i++) {
        printf("SCC %d: 大小 = %d, 包含节点: ", i, scc_size[i]);
        for (int j = 1; j <= n; j++) {
            if (scc[j] == i) printf("%d ", j);
        }
        printf("\n");
    }
    */

    // 2. 求割点
    for (int i = 1; i <= n; i++) {
        if (!dfn[i]) tarjan_cut_point(i, 0);
    }
    printf("\n割点: ");
    for (int i = 1; i <= n; i++) {
        if (cut_point[i]) printf("%d ", i);
    }
    printf("\n");

    // 3. 求割边（需要重新初始化 dfn/low）
    memset(dfn, 0, sizeof(dfn));
    memset(low, 0, sizeof(low));
    dfncnt = 0;
    for (int i = 1; i <= n; i++) {
        if (!dfn[i]) tarjan_bridge(i, 0);
    }
    printf("\n割边（桥）:\n");
    for (int i = 1; i <= bridge_cnt; i++) {
        printf("%d - %d\n", bridge[i].u, bridge[i].v);
    }

    return 0;
}