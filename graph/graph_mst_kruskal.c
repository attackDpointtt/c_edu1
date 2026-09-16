#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 100010   // 最大节点数
#define MAXM 500010   // 最大边数

// ==================== 并查集 ====================
int fa[MAXN], rnk[MAXN];  // fa: 父节点, rnk: 秩（用于按秩合并）

// 初始化并查集
void init_uf(int n) {
    for (int i = 1; i <= n; i++) {
        fa[i] = i;
        rnk[i] = 0;
    }
}

// 查找根节点（路径压缩）
int find(int x) {
    if (fa[x] != x)
        fa[x] = find(fa[x]);
    return fa[x];
}

// 合并两个集合（按秩合并），返回是否合并成功
int unite(int x, int y) {
    int rx = find(x), ry = find(y);
    if (rx == ry) return 0;  // 已在同一集合，合并失败

    // 按秩合并：将秩小的树挂到秩大的树上
    if (rnk[rx] < rnk[ry]) {
        fa[rx] = ry;
    } else if (rnk[rx] > rnk[ry]) {
        fa[ry] = rx;
    } else {
        fa[ry] = rx;
        rnk[rx]++;
    }
    return 1;
}

// ==================== Kruskal 算法 ====================
typedef struct {
    int u, v, w;  // 起点、终点、边权
} Edge;

Edge edges[MAXM];
int n, m;  // 节点数、边数

// 排序比较函数（按边权升序）
int cmp(const void *a, const void *b) {
    return ((Edge *)a)->w - ((Edge *)b)->w;
}

int kruskal() {
    init_uf(n);

    // 1. 将所有边按权值从小到大排序
    qsort(edges, m, sizeof(Edge), cmp);

    int mst_weight = 0;  // 最小生成树的总权值
    int edge_count = 0;  // 已选边数

    // 2. 遍历排序后的边
    for (int i = 0; i < m; i++) {
        int u = edges[i].u, v = edges[i].v, w = edges[i].w;

        // 3. 如果 u 和 v 不在同一集合，则加入该边
        if (unite(u, v)) {
            mst_weight += w;
            edge_count++;
            printf("选中边: %d - %d, 权值: %d\n", u, v, w);

            // 4. 选够 n-1 条边即可提前结束
            if (edge_count == n - 1) break;
        }
    }

    // 5. 判断是否构成生成树（连通图）
    if (edge_count < n - 1) {
        printf("图不连通，无法构成最小生成树！\n");
        return -1;
    }

    return mst_weight;
}

// ==================== 主函数 ====================
int main() {
    printf("请输入节点数和边数: ");
    scanf("%d %d", &n, &m);

    printf("请输入 %d 条边 (u v w):\n", m);
    for (int i = 0; i < m; i++) {
        scanf("%d %d %d", &edges[i].u, &edges[i].v, &edges[i].w);
    }

    printf("\n--- Kruskal 算法执行过程 ---\n");
    int result = kruskal();

    if (result != -1) {
        printf("\n最小生成树总权值: %d\n", result);
    }

    return 0;
}