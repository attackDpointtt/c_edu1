#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "sort.h"

#define ARRAY_SIZE 100
#define ROUNDS 20

void main(void) {
    // 扫描当前目录，列出可选排序文件
      
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    char files[20][256];
    int fileCount = 0;

    hFind = FindFirstFile("*.c", &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("无法扫描目录\n");
        return 1;
    }

    do {
        if (strcmp(findData.cFileName, "main.c") != 0) {
            strcpy(files[fileCount], findData.cFileName);
            printf("[%d] %s\n", fileCount + 1, files[fileCount]);
            fileCount++;
        }
    } while (FindNextFile(hFind, &findData) && fileCount < 20);
    FindClose(hFind);

    if (fileCount == 0) {
        printf("未找到排序文件\n");
        return 1;
    }

    while (1) {  // 用户选择
        int choice;
        printf("choose algorithm (1-%d): ", fileCount);
        scanf("%d", &choice);

        if (choice < 1 || choice > fileCount) {
            printf("invalid choice\n");
            return 1;
        }

        char *selectedFile = files[choice - 1];
        printf("\ntest file: %s\n", selectedFile);

        // 测试循环
        srand((unsigned int)time(NULL));
        int arr[ARRAY_SIZE];
        int correct = 1;
        int round;

        for (round = 1; round <= ROUNDS; round++) {
            // 生成随机数组
            int i;
            for (i = 0; i < ARRAY_SIZE; i++) {
                arr[i] = rand() % 1000 - 500;
            }

            // 计时 + 排序
            clock_t start = clock();

            if (strstr(selectedFile, "bubble"))
                bubbleSort(arr, ARRAY_SIZE);
            else if (strstr(selectedFile, "choose"))
                selectionSort(arr, ARRAY_SIZE);
            else if (strstr(selectedFile, "insert"))
                insertionSort(arr, ARRAY_SIZE);

            clock_t end = clock();

            // 升序验证
            int sorted = 1;
            for (i = 0; i < ARRAY_SIZE - 1; i++) {
                if (arr[i] > arr[i + 1]) {
                    sorted = 0;
                    break;
                }
            }

            double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
            printf("/%s/range %s  time: %fs\n",
                selectedFile,
                sorted ? "correct" : "incorrect",
                elapsed);

            if (!sorted) {
                correct = 0;
                break;
            }
        }

        if (correct)
            printf("\n %d round correct,pass!\n", ROUNDS);
        else
            printf("\n %d round failed,fail!\n", round);

        

    }
}