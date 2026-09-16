#include <stdio.h>

void cycleSort(int arr[], int n) {
    for (int cycleStart = 0; cycleStart < n - 1; cycleStart++) {
        int item = arr[cycleStart];
        int pos = cycleStart;

        for (int i = cycleStart + 1; i < n; i++) {
            if (arr[i] < item) {
                pos++;
            }
        }

        if (pos == cycleStart) {
            continue;
        }

        while (item == arr[pos]) {
            pos++;
        }

        int temp = arr[pos];
        arr[pos] = item;
        item = temp;
    }
}

void printArray(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main() {
    int arr[] = {3, 2, 1, 5, 4};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("排序前: ");
    printArray(arr, n);

    cycleSort(arr, n);

    printf("排序后: ");
    printArray(arr, n);

    return 0;
}