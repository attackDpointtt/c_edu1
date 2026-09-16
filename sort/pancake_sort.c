#include <stdio.h>

void flip(int arr[], int i) {
    int temp;
    for (int j = 0; j < i; j++) {
        temp = arr[j];
        arr[j] = arr[i];
        arr[i] = temp;
    }
}

int findMax(int arr[], int n) {
    int maxIndex = 0;
    for (int i = 1; i < n; i++) {
        if (arr[i] > arr[maxIndex]) {
            maxIndex = i;
        }
    }
    return maxIndex;
}

void pancakeSort(int arr[], int n) {
    for (int size = n; size > 1; size--) {
        int maxIndex = findMax(arr, size);
        if (maxIndex != size - 1) {
            flip(arr, maxIndex + 1);
            flip(arr, size);
        }
    }
}

void printArray(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main() {
    int arr[] = {23, 10, 20, 11, 12};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("排序前: ");
    printArray(arr, n);

    pancakeSort(arr, n);

    printf("排序后: ");
    printArray(arr, n);

    return 0;
}
