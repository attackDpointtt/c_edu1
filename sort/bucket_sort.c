



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 桶排序实现（支持整数和浮点数）
 * 核心逻辑：
 * 1. 将待排序数组元素按映射函数分配到多个桶中
 * 2. 对每个桶内的元素进行排序（此处使用插入排序）
 * 3. 按顺序合并所有桶中的元素，得到最终有序数组
 */

// 插入排序（用于桶内排序）
void insertionSort(double arr[], int n) {
    for (int i = 1; i < n; i++) {
        double key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// 桶排序主函数
void bucketSort(double arr[], int n, int bucketCount) {
    if (n <= 1 || bucketCount <= 0) return;

    // 1. 找到数组的最大值和最小值
    double max = arr[0], min = arr[0];
    for (int i = 1; i < n; i++) {
        if (arr[i] > max) max = arr[i];
        if (arr[i] < min) min = arr[i];
    }

    // 2. 计算桶的范围（避免除零错误）
    double range = max - min;
    if (range == 0) return; // 所有元素相同，无需排序

    // 3. 创建桶（每个桶是一个动态数组）
    double** buckets = (double**)malloc(bucketCount * sizeof(double*));
    int* bucketSizes = (int*)calloc(bucketCount, sizeof(int));
    int* bucketCapacities = (int*)malloc(bucketCount * sizeof(int));

    for (int i = 0; i < bucketCount; i++) {
        bucketCapacities[i] = 10; // 初始容量
        buckets[i] = (double*)malloc(bucketCapacities[i] * sizeof(double));
    }

    // 4. 将元素分配到桶中
    for (int i = 0; i < n; i++) {
        // 计算元素应放入的桶索引
        int index = (int)((arr[i] - min) / range * (bucketCount - 1));
        if (index >= bucketCount) index = bucketCount - 1; // 处理最大值边界

        // 如果桶满，扩容
        if (bucketSizes[index] >= bucketCapacities[index]) {
            bucketCapacities[index] *= 2;
            buckets[index] = (double*)realloc(buckets[index], bucketCapacities[index] * sizeof(double));
        }

        buckets[index][bucketSizes[index]++] = arr[i];
    }

    // 5. 对每个桶进行插入排序，并合并回原数组
    int idx = 0;
    for (int i = 0; i < bucketCount; i++) {
        insertionSort(buckets[i], bucketSizes[i]);
        for (int j = 0; j < bucketSizes[i]; j++) {
            arr[idx++] = buckets[i][j];
        }
    }

    // 6. 释放内存
    for (int i = 0; i < bucketCount; i++) {
        free(buckets[i]);
    }
    free(buckets);
    free(bucketSizes);
    free(bucketCapacities);
}

// 打印数组
void printArray(double arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%.2f ", arr[i]);
    }
    printf("\n");
}

// 测试函数
int main() {
    // 测试1：浮点数排序
    double arr1[] = {0.78, 0.17, 0.39, 0.26, 0.72, 0.94, 0.21, 0.12, 0.23, 0.68};
    int n1 = sizeof(arr1) / sizeof(arr1[0]);

    printf("原始数组: ");
    printArray(arr1, n1);

    bucketSort(arr1, n1, 10);

    printf("排序后:   ");
    printArray(arr1, n1);

    // 测试2：整数排序（转换为double处理）
    double arr2[] = {37, 12, 89, 5, 23, 45, 67, 1, 99, 34};
    int n2 = sizeof(arr2) / sizeof(arr2[0]);

    printf("\n原始数组: ");
    printArray(arr2, n2);

    bucketSort(arr2, n2, 10);

    printf("排序后:   ");
    printArray(arr2, n2);

    return 0;
}