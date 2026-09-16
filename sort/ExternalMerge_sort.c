#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * 外部归并排序（External Merge Sort）
 * 
 * 适用场景：数据量远超内存容量，无法一次性载入内存排序
 * 核心思想：分治 + 归并
 *   阶段1：将大文件分块，每块在内存中排序后写回磁盘（生成归并段）
 *   阶段2：多路归并所有归并段，得到最终有序文件
 * ============================================================ */

#define MEM_BLOCK_SIZE 10000  // 每次读入内存的最大元素数（模拟内存限制）
#define K_WAY 4               // 归并路数（k路归并）
#define TMP_DIR "./tmp/"      // 临时文件目录

/* ===================== 内部排序：快速排序 ===================== */

int cmp_int(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

/* 对数组 arr[0..n-1] 进行快速排序 */
void quick_sort(int arr[], int n) {
    if (n <= 1) return;
    qsort(arr, n, sizeof(int), cmp_int);
}

/* ===================== 最小堆（用于多路归并） ===================== */

/*
 * 堆中每个元素记录：
 *   value   - 当前元素的值
 *   file_id - 该元素来自哪个归并段文件
 */
typedef struct {
    int value;
    int file_id;
} HeapNode;

/* 最小堆结构 */
typedef struct {
    HeapNode *data;
    int size;
    int capacity;
} MinHeap;

/* 创建最小堆 */
MinHeap *heap_create(int capacity) {
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    heap->data = (HeapNode *)malloc(sizeof(HeapNode) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

/* 释放最小堆 */
void heap_destroy(MinHeap *heap) {
    free(heap->data);
    free(heap);
}

/* 上浮调整 */
void heap_sift_up(MinHeap *heap, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (heap->data[parent].value > heap->data[idx].value) {
            HeapNode tmp = heap->data[parent];
            heap->data[parent] = heap->data[idx];
            heap->data[idx] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

/* 下沉调整 */
void heap_sift_down(MinHeap *heap, int idx) {
    while (1) {
        int smallest = idx;
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;
        if (left < heap->size && heap->data[left].value < heap->data[smallest].value)
            smallest = left;
        if (right < heap->size && heap->data[right].value < heap->data[smallest].value)
            smallest = right;
        if (smallest != idx) {
            HeapNode tmp = heap->data[smallest];
            heap->data[smallest] = heap->data[idx];
            heap->data[idx] = tmp;
            idx = smallest;
        } else {
            break;
        }
    }
}

/* 插入元素 */
void heap_push(MinHeap *heap, int value, int file_id) {
    assert(heap->size < heap->capacity);
    heap->data[heap->size].value = value;
    heap->data[heap->size].file_id = file_id;
    heap->size++;
    heap_sift_up(heap, heap->size - 1);
}

/* 弹出堆顶（最小值） */
HeapNode heap_pop(MinHeap *heap) {
    assert(heap->size > 0);
    HeapNode top = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    if (heap->size > 0)
        heap_sift_down(heap, 0);
    return top;
}

/* ===================== 阶段1：生成初始归并段 ===================== */

/*
 * 将输入文件分块读入内存，每块排序后写入一个临时文件
 * 返回生成的归并段数量
 */
int generate_runs(const char *input_file, char run_files[][100]) {
    FILE *fin = fopen(input_file, "r");
    if (!fin) {
        perror("无法打开输入文件");
        exit(EXIT_FAILURE);
    }

    int *buffer = (int *)malloc(sizeof(int) * MEM_BLOCK_SIZE);
    if (!buffer) {
        perror("内存分配失败");
        exit(EXIT_FAILURE);
    }

    int run_count = 0;
    int count = 0;
    int num;

    while (fscanf(fin, "%d", &num) == 1) {
        buffer[count++] = num;

        // 缓冲区满，排序并写回磁盘
        if (count == MEM_BLOCK_SIZE) {
            quick_sort(buffer, count);

            char filename[100];
            sprintf(filename, "%srun_%d.dat", TMP_DIR, run_count);
            FILE *fout = fopen(filename, "w");
            if (!fout) {
                perror("无法创建临时文件");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < count; i++) {
                fprintf(fout, "%d\n", buffer[i]);
            }
            fclose(fout);

            strcpy(run_files[run_count], filename);
            run_count++;
            count = 0;
        }
    }

    // 处理最后一块不足 MEM_BLOCK_SIZE 的数据
    if (count > 0) {
        quick_sort(buffer, count);

        char filename[100];
        sprintf(filename, "%srun_%d.dat", TMP_DIR, run_count);
        FILE *fout = fopen(filename, "w");
        if (!fout) {
            perror("无法创建临时文件");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < count; i++) {
            fprintf(fout, "%d\n", buffer[i]);
        }
        fclose(fout);

        strcpy(run_files[run_count], filename);
        run_count++;
    }

    free(buffer);
    fclose(fin);
    printf("[阶段1] 生成了 %d 个初始归并段\n", run_count);
    return run_count;
}

/* ===================== 阶段2：K路归并 ===================== */

/*
 * 将 run_count 个归并段进行 K 路归并，结果写入 output_file
 */
void k_way_merge(char run_files[][100], int run_count, const char *output_file) {
    FILE *fout = fopen(output_file, "w");
    if (!fout) {
        perror("无法打开输出文件");
        exit(EXIT_FAILURE);
    }

    // 为每个归并段打开文件指针
    FILE **files = (FILE **)malloc(sizeof(FILE *) * run_count);
    int *current = (int *)malloc(sizeof(int) * run_count);       // 当前读取的值
    int *has_next = (int *)malloc(sizeof(int) * run_count);      // 是否还有数据

    for (int i = 0; i < run_count; i++) {
        files[i] = fopen(run_files[i], "r");
        if (!files[i]) {
            perror("无法打开归并段文件");
            exit(EXIT_FAILURE);
        }
        // 预读第一个元素
        if (fscanf(files[i], "%d", &current[i]) == 1) {
            has_next[i] = 1;
        } else {
            has_next[i] = 0;
        }
    }

    // 创建最小堆，容量为归并段数量
    MinHeap *heap = heap_create(run_count);

    // 初始化堆：将每个归并段的第一个元素入堆
    for (int i = 0; i < run_count; i++) {
        if (has_next[i]) {
            heap_push(heap, current[i], i);
        }
    }

    // 不断弹出最小值，并从对应文件补充新元素
    while (heap->size > 0) {
        HeapNode node = heap_pop(heap);
        fprintf(fout, "%d\n", node.value);

        int fid = node.file_id;
        // 从对应文件读取下一个元素
        if (fscanf(files[fid], "%d", &current[fid]) == 1) {
            heap_push(heap, current[fid], fid);
        } else {
            has_next[fid] = 0;
        }
    }

    // 清理资源
    heap_destroy(heap);
    for (int i = 0; i < run_count; i++) {
        fclose(files[i]);
    }
    fclose(fout);
    free(files);
    free(current);
    free(has_next);

    printf("[阶段2] K路归并完成，结果已写入 %s\n", output_file);
}

/* ===================== 主函数 ===================== */

int main() {
    const char *input_file = "input.txt";
    const char *output_file = "output.txt";

    // 创建临时目录（实际项目中可用 mkdir）
    system("mkdir -p " TMP_DIR);

    printf("========== 外部归并排序 ==========\n");
    printf("输入文件: %s\n", input_file);
    printf("内存块大小: %d 个整数\n", MEM_BLOCK_SIZE);
    printf("归并路数: %d 路\n\n", K_WAY);

    // 阶段1：生成初始归并段
    char run_files[1000][100];  // 最多支持1000个归并段
    int run_count = generate_runs(input_file, run_files);

    if (run_count == 0) {
        printf("输入文件为空，无需排序。\n");
        return 0;
    }

    // 阶段2：K路归并
    k_way_merge(run_files, run_count, output_file);

    printf("\n排序完成！结果保存在: %s\n", output_file);

    // 清理临时文件（可选）
    // system("rm -rf " TMP_DIR);

    return 0;
}