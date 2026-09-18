void* malloc(size_t 字节数);
对比文件中的node*，
node* 指向某一node结构体，
void* 不指向某一特定类型，随时可以转换。

| 问题      | 答案                      |
| ------- | ----------------------- |
| 参数是什么   | **要多少字节**（不是"几个"）       |
| 返回什么    | 一块内存的**地址**；失败返回 `NULL` |
| 头文件     | `<stdlib.h>`            |
| 会不会自动清零 | ❌ 不会，里面是垃圾值             |
| 谁负责释放   | **你**，用 `free`          |
| 住在哪     | 堆                       |
// 1. 申请
Node* p = (Node*)malloc(sizeof(Node));

// 2. 检查
if (p == NULL) {
    return NULL;              // 申请失败，别往下走
}

// 3. 初始化（关键，别省）
p->data  = 5;
p->left  = NULL;
p->right = NULL;

// 4. 使用
printf("%d\n", p->data);

// 5. 释放
free(p);
p = NULL;                     // 好习惯：防止野指针