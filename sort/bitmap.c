#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Bitmap 位图排序
 * 
 * 适用前提：
 *   1. 数据为不重复的非负整数
 *   2. 已知数据范围上限 MAX_VAL
 * 
 * 核心思想：
 *   用 1 bit 标记一个整数是否存在，bit 下标 = 整数值
 *   初始化全0 → 读入数据置1 → 顺序扫描输出 = 自动有序
 */

#define MAX_VAL    10000000   // 数据上限（一千万）
#define WORD       32         // 每个 int 占 32 bit
#define SHIFT      5          // 2^5 = 32，右移5位等价于 /32
#define MASK       0x1F       // 0x1F = 31，与操作等价于 %32

// 位图数组：1 + MAX_VAL/32 个 int，共约 1.25MB
int bitmap[1 + MAX_VAL / WORD];

/* 将第 i 位置 1 */
void set_bit(int i) {
    bitmap[i >> SHIFT] |= (1 << (i & MASK));
}

/* 将第 i 位清 0 */
void clear_bit(int i) {
    bitmap[i >> SHIFT] &= ~(1 << (i & MASK));
}

/* 测试第 i 位是否为 1 */
int test_bit(int i) {
    return bitmap[i >> SHIFT] & (1 << (i & MASK));
}

int main(void) {
    int i, n;

    // 阶段1：初始化，所有位清0
    memset(bitmap, 0, sizeof(bitmap));

    // 阶段2：读入数据，将对应位置1（自动去重）
    printf("请输入待排序的非负整数（Ctrl+D 结束）：\n");
    while (scanf("%d", &n) != EOF) {
        if (n < 0 || n >= MAX_VAL) {
            fprintf(stderr, "警告: %d 超出范围 [0, %d)，已忽略\n", n, MAX_VAL - 1);
            continue;
        }
        set_bit(n);
    }

    // 阶段3：顺序扫描，输出所有为1的位下标 → 天然升序
    printf("排序结果：\n");
    for (i = 0; i < MAX_VAL; i++) {
        if (test_bit(i)) {
            printf("%d ", i);
        }
    }
    printf("\n");

    return 0;
}