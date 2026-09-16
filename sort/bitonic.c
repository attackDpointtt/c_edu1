#include <stdio.h>
#include <stdlib.h>

/*
 * 双调排序 (Bitonic Sort)
 * 
 * 前提：数组长度必须为 2 的幂
 * 
 * 核心思想：
 *   1. 递归构建双调序列：前半升序 + 后半降序 → 合并为双调序列
 *   2. 双调归并：将双调序列拆分为 MIN/MAX 两半，递归归并
 */

#define ASCENDING  1
#define DESCENDING 0

/* 比较并交换：若 (a[i] > a[j]) 与 dir 一致，则交换 */
void compAndSwap(int a[], int i, int j, int dir) {
    if (dir == (a[i] > a[j])) {
        int tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

/*
 * 双调归并：将 a[low..low+cnt-1] 这个双调序列按 dir 方向归并为有序序列
 */
void bitonicMerge(int a[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        // 第一步：跨半比较，将序列拆分为 MIN 和 MAX 两半
        for (int i = low; i < low + k; i++) {
            compAndSwap(a, i, i + k, dir);
        }
        // 第二步：递归归并两半
        bitonicMerge(a, low, k, dir);
        bitonicMerge(a, low + k, k, dir);
    }
}

/*
 * 双调排序：递归构建双调序列，然后归并
 * dir=1 升序, dir=0 降序
 */
void bitonicSort(int a[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        // 前半升序，后半降序 → 拼接后形成双调序列
        bitonicSort(a, low, k, ASCENDING);
        bitonicSort(a, low + k, k, DESCENDING);
        // 将整个双调序列按 dir 方向归并
        bitonicMerge(a, low, cnt, dir);
    }
}

/* 对外接口：排序整个数组 */
void sort(int a[], int n, int up) {
    bitonicSort(a, 0, n, up);
}

/* 打印数组 */
void printArray(int a[], int n) {
    for (int i = 0; i < n; i++)
        printf("%d ", a[i]);
    printf("\n");
}

int main() {
    int a[] = {3, 7, 4, 8, 6, 2, 1, 5};
    int n = sizeof(a) / sizeof(a[0]);

    printf("排序前: ");
    printArray(a, n);

    sort(a, n, ASCENDING);

    printf("排序后: ");
    printArray(a, n);

    return 0;
}