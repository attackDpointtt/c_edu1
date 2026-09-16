#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * TimSort - 归并排序 + 插入排序的混合稳定排序
 * 
 * 核心思想：
 *   1. 将数组划分为多个有序小段（run）
 *   2. 短 run 用插入排序扩展到 minRun 长度
 *   3. 用归并排序合并所有 run
 */

#define MIN_MERGE 32  // 最小归并长度

// 计算最小 run 长度，保证 n/minRun 接近 2 的幂
int calcMinRun(int n) {
    int r = 0;
    while (n >= MIN_MERGE) {
        r |= (n & 1);   // 记录最低位
        n >>= 1;         // 右移一位
    }
    return n + r;
}

// 插入排序：对 arr[left..right] 进行排序
void insertionSort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// 归并：合并 arr[left..mid] 和 arr[mid+1..right]
void merge(int arr[], int left, int mid, int right) {
    int len1 = mid - left + 1;
    int len2 = right - mid;

    // 创建临时数组
    int *L = (int *)malloc(len1 * sizeof(int));
    int *R = (int *)malloc(len2 * sizeof(int));

    for (int i = 0; i < len1; i++)
        L[i] = arr[left + i];
    for (int j = 0; j < len2; j++)
        R[j] = arr[mid + 1 + j];

    // 合并回原数组
    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2) {
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }
    while (i < len1) arr[k++] = L[i++];
    while (j < len2) arr[k++] = R[j++];

    free(L);
    free(R);
}

// TimSort 主函数
void timSort(int arr[], int n) {
    if (n <= 1) return;

    int minRun = calcMinRun(n);

    // 阶段1：对每个长度为 minRun 的小段做插入排序
    for (int i = 0; i < n; i += minRun) {
        int end = (i + minRun - 1 < n - 1) ? (i + minRun - 1) : (n - 1);
        insertionSort(arr, i, end);
    }

    // 阶段2：逐步合并 run，每次合并大小翻倍
    for (int size = minRun; size < n; size *= 2) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n - 1) ? (left + 2 * size - 1) : (n - 1);

            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }
}

// 打印数组
void printArray(int arr[], int n) {
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

int main() {
    int arr[] = {5, 21, 7, 23, 19, 1, 4, 8, 12, 3, 15, 6, 10, 2, 11, 9,
                 14, 17, 13, 16, 18, 20, 22, 24};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("排序前: ");
    printArray(arr, n);

    timSort(arr, n);

    printf("排序后: ");
    printArray(arr, n);

    return 0;
}