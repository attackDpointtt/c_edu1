#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sleepSort(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        sleep(arr[i]);
        printf("%d ", arr[i]);
    }
}

int main() {
    int arr[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    int n = sizeof(arr) / sizeof(arr[0]);
    printf("Original array: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    printf("Sorted array: ");
    sleepSort(arr, n);
    printf("\n");
    return 0;
}