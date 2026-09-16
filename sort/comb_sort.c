#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * 梳排序 (Comb Sort)
 * 
 * 核心逻辑：
 * 1. 初始化间距 gap = n
 * 2. 每次循环将 gap 缩小为 gap / 1.3 (收缩因子)
 * 3. 当 gap >= 1 时，对间距为 gap 的元素进行类似冒泡的比较交换
 * 4. 当 gap == 1 且没有发生任何交换时，排序完成
 */
void combSort(int arr[], int n) {
    if (n <= 1) return;

    // 收缩因子，经验值 1.3
    const double shrink_factor = 1.3;
    int gap = n;
    int swapped = 1; // 标记是否发生交换，1表示是，0表示否

    // 当 gap > 1 或者最近一次遍历发生了交换时，继续循环
    while (gap > 1 || swapped) {
        // 缩小间距
        gap = (int)(gap / shrink_factor);
        if (gap < 1) gap = 1;

        swapped = 0; // 重置交换标记

        // 对间距为 gap 的元素进行比较和交换
        for (int i = 0; i + gap < n; i++) {
            if (arr[i] > arr[i + gap]) {
                // 交换元素
                int temp = arr[i];
                arr[i] = arr[i + gap];
                arr[i + gap] = temp;
                swapped = 1;
            }
        }
    }
}

// 打印数组
void printArray(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

// 验证数组是否有序
int isSorted(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) return 0;
    }
    return 1;
}

int main() {
    // 测试1：基本功能测试
    printf("========== 测试1：基本功能 ==========\n");
    int arr1[] = {8, 4, 1, 56, 3, -44, 23, -6, 28, 0};
    int n1 = sizeof(arr1) / sizeof(arr1[0]);

    printf("排序前: ");
    printArray(arr1, n1);

    combSort(arr1, n1);

    printf("排序后: ");
    printArray(arr1, n1);
    printf("是否有序: %s\n\n", isSorted(arr1, n1) ? "是" : "否");

    // 测试2：大规模随机数据性能测试
    printf("========== 测试2：性能测试 (10万个随机整数) ==========\n");
    int n2 = 100000;
    int *arr2 = (int *)malloc(n2 * sizeof(int));
    if (!arr2) {
        perror("内存分配失败");
        return 1;
    }

    // 生成随机数据
    srand(time(NULL));
    for (int i = 0; i < n2; i++) {
        arr2[i] = rand() % 1000000;
    }

    clock_t start = clock();
    combSort(arr2, n2);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("排序耗时: %.4f 秒\n", time_spent);
    printf("是否有序: %s\n", isSorted(arr2, n2) ? "是" : "否");

    free(arr2);

    return 0;
}