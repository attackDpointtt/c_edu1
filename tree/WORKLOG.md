# 工作日志（AI 操作记录）

> **用途**：简要记录 AI 助手在本项目中做过的改动，方便回溯。
> **规则**：只记「改了什么 + 为什么 + 怎么验证」，不贴大段代码。
> **回滚**：改动前的原文件均留有 `.bak` 备份，直接改回后缀即可恢复。

---

## 2026-09-17

### 0. 创建本日志文件

| 项 | 内容 |
|---|---|
| 操作 | 创建 `WORKLOG.md`（即本文件） |
| 原因 | 应要求，需要一个文件简要记录 AI 对本项目的所有改动 |
| 规则 | 只记「改了什么 + 为什么 + 怎么验证」，不贴大段代码 |

### 1. 备份 `Btree.c`

| 项 | 内容 |
|---|---|
| 操作 | `Btree.c` → 重命名为 `Btree.c.bak` |
| 原因 | 接下来要大幅重写，先留一份原始代码 |
| 验证 | 目录中确认出现 `Btree.c.bak` |

### 2. 重写 `Btree.c`，使其成为真正的 B 树

**原因**：原实现既不满足 B 树的性质，也无法通过编译。

**修复的 5 个问题：**

| # | 问题 | 后果 | 修法 |
|---|---|---|---|
| 1 | `merge` / `deleteKey` 先用后定义 | 编译报 `conflicting types` 错误，直接编不过 | 文件开头统一加前向声明 |
| 2 | `insert` 遇到空树时只造节点、丢掉 key | 第一个插入的数据凭空消失 | 造根时把 key 一并存进去 |
| 3 | `mergeChildren` 拿「关键字个数」当「孩子下标」用 | 数组越界写入，`num_keys` 被写坏 | 改用合并前的基点 `base + i` 算下标 |
| 4 | 删内部节点关键字时没有递归删前驱/后继 | 子树关键字数掉到下限以下，破坏 B 树性质 | 搬完后递归调用 `deleteKeyRec` 真删 |
| 5 | `search` 没有判空 | 对空树查找会解引用 NULL 崩溃 | 入口加 `if (node == NULL) return false;` |

**新增内容：**

- `validateTree()` / `validateSubtree()` —— 递归校验 B 树的 5 条性质
- 根节点收缩逻辑（删除后树高自动降低）
- 完整的中文注释

**验证：**

| 检查项 | 结果 |
|---|---|
| `gcc -Wall -Wextra -std=c11` 编译 | 零警告 |
| 运行操作次数 | 16 次（10 次插入 + 6 次删除） |
| B 树性质校验 | 16 次全部 PASS，0 次 FAIL |
| 中序遍历有序性 | 每步均正确 |

### 3. 程序输出提示改为英文

| 项 | 内容 |
|---|---|
| 操作 | 16 处 `printf` / `snprintf` 的中文提示改为英文 |
| 原因 | 应要求；顺带消除终端显示乱码 |
| 保留 | 代码注释仍为中文（作为学习笔记） |
| 验证 | 运行输出 158 行，含非 ASCII 字符的行数 = 0 |

### 4. 重写 `bst.c`（BST 学习用）

| 项 | 内容 |
|---|---|
| 操作 | 备份 `bst.c` → `bst.c.bak`，重写为完整可运行的 BST 演示 |
| 原因 | 原文件仅 37 行（只有 `createNode` / `insertNode` / `searchNode`），没有删除、遍历、`main`，不够学 |
| 新增内容 | 删除（叶子 / 单孩子 / 双孩子三种情况）、四种遍历（前序/中序/后序/层序）、`findMin`/`findMax`、树高与节点数、`isBST` 区间校验、树形打印、`main` 演示 |
| 输出语言 | 英文（与 `Btree.c` 统一，避免终端乱码）；注释为中文 |
| 验证 | 编译零警告；运行 113 行输出，非 ASCII 行数 = 0；所有状态 `Is valid BST: YES` |

**演示流程**：建树 → 查找 → 删除三种情况各一次 → 插入有序数据导致树退化成链（6 个节点、树高 6），作为学 B 树的动机。

### 5. 操作失误记录

| 项 | 内容 |
|---|---|
| 事故 | 对 `bst.c` **同时发起 3 个编辑**，内容被互相冲掉导致文件损坏，编译报 `'rooTree' undeclared` |
| 处理 | 删除损坏文件后整体重建，重新编译运行验证通过 |
| 教训 | **同一个文件不要并行编辑，必须串行逐条修改** |


---

## 2026-09-18

### 6. 重写 `red-black_tree.c`（红黑树学习用）

| 项 | 内容 |
|---|---|
| 操作 | 备份 `red-black_tree.c` → `red-black_tree.c.bak`，在原有定义基础上补全为完整可运行的红黑树演示 |
| 原因 | 原文件仅 475 字节（只有 `Color` / `Node` / `createNode`），没有旋转、插入、删除、`main`，学不了 |
| 保留 | 原有三个定义**逐字未改**，只补注释（含「新节点为何入红」「为何必须有 parent」） |
| 新增 | 旋转（`rotateLeft` / `rotateRight`，全程维护 `parent`）、插入 + `insertFixup`（三种情况）、删除 + `deleteFixup`（四种情况，NULL 双黑靠 `xParent` 回溯）、查找、`validateTree`（五条性质 + BST 有序性 + parent 一致性）、中序 / 层序 / 带颜色树形打印、`main` 演示 |
| 演示流程 | 递升插入 6 键（对比 BST 会退化成高度 6 的链）→ 查找 → 删除 7 步 → 200 随机插入 + 200 随机删除压力测试 |
| 输出语言 | 英文（注释中文，与项目约定一致） |
| 验证 | `gcc -Wall -Wextra -std=c11` 零警告；删除演示 7 步均 `valid RBT: YES`；压力测试**PASS 405 / FAIL 0**，中序始终升序；200 节点树高 9（理论上界 15） |

**踩到的两个坑**（下次注意）：

| # | 坑 | 后果 | 避免方法 |
|---|---|---|---|
| 1 | 旧文件**末尾没有换行符**，但匹配串带了 `\n` | `replace_string` 匹配失败 | 改整个文件前先确认末尾字节 |
| 2 | 匹配串只取了 `createNode`，漏掉前面已有的 `Color` / `Node` 定义 | 重复定义，编译报 6 个错误 | 替换前先通读待改区域，别只看函数体 |

### 7. 补充 `avl_tree.c`（AVL 树学习用）

| 项 | 内容 |
|---|---|
| 操作 | 备份 `avl_tree.c` → `avl_tree.c.bak`；**原有逻辑一行未改**，只补注释并在后面追加演示 |
| 原因 | 原文件逻辑正确、编译零警告，但只有一个 20 行的 `main`，没有校验、没有释放、没有压力测试 |
| 新增 | `search` / `countNodes` / `validateAVL`（BST 有序性 + height 字段是否漂移 + 平衡因子 ≤ 1）/ 带 `h` `bf` 的树形打印 / `destroyTree` / 300 次随机增删压力测试 |
| 输出语言 | 英文（注释中文） |
| 验证 | 编译零警告；605 次校验 **PASS 605 / FAIL 0**；300 节点树高 10（AVL 理论上界约 12） |

### 8. 修复 `B+tree.c`（原文件无法编译，且有 4 处结构性 bug）

| 项 | 内容 |
|---|---|
| 操作 | 备份 `B+tree.c` → `B+tree.c.bak`，修复 bug 并补齐校验与演示 |
| 原因 | 原文件**编译不过**，且即使编过也会静默丢数据 |

**修复的 6 个问题：**

| # | 问题 | 后果 | 修法 |
|---|---|---|---|
| 1 | `removeFromNonLeaf` 调用 `merge` / `deleteKey` 但二者定义在后，无前向声明 | `implicit declaration` → `conflicting types`，**编译直接失败** | 文件开头统一加函数原型 |
| 2 | `insert` 遇空树时只 `createNode` 不存 key | **第一个插入的关键字凭空消失** | 造根后调用 `insertNonFull` 存入 key |
| 3 | `merge` 复制孩子指针时在循环里 `left->num_keys++` | 把「关键字个数」当「孩子下标」用，节点结构被写坏 | 改用合并前基点 `leftKeys + 1 + i` 算下标 |
| 4 | `borrowFromRight` 孩子左移循环只跑 `num_keys` 次 | **最后一棵子树被写 NULL 丢弃**（本次实测丢了 9 个关键字） | 循环条件改为 `i <= num_keys`（孩子个数 = 关键字数 + 1） |
| 5 | `removeFromNonLeaf` 搬完前驱/后继后直接 `removeFromLeaf` | 跳过了下溢修复，叶子关键字数掉到下限以下 | 改为递归调用 `deleteKey` 真正删除替身 |
| 6 | `deleteKey` 丢弃递归返回值、且根删空后不收缩 | 父节点指向已 free 的旧节点（use-after-free）→ 段错误；根留下「0 关键字的空叶子」 | 接回返回值 `node->children[i] = deleteKey(...)`；末尾统一收缩根 |
| 7 | `search` 未判空；`createNode` 用 `perror` 输出中文 | 空树查找崩溃；终端乱码 | 加 `if (node == NULL) return false;`；输出改英文 |

**新增内容**：`validateTree`（5 条 B 树性质：关键字个数上下限、节点内升序、子树区间、孩子个数、叶子同层）、`countKeys`、`treeHeight`、以及 500 次随机插入 + 500 次随机删除的压力测试。

**验证**：编译零警告；**PASS 1005 / FAIL 0**；中序始终升序；500 个关键字树高仅 7 层；每次删除后关键字总数恰好减 1。

### 9. 事故记录（我自己造成的，已恢复）

| 项 | 内容 |
|---|---|
| 事故 1 | 把 `B+tree.c` 编译输出命名为 `btree_demo.exe`，而 Windows **文件名不区分大小写**，直接覆盖了原有的 `Btree_demo.exe` |
| 处理 1 | 新产物改名 `Bplus_demo.exe`；用 `Btree.c` 重新编译 `Btree_demo.exe` 恢复（63248 字节，与原值一致） |
| 事故 2 | 顺手重新编译了 `Btree.exe`，覆盖掉原有 72680 字节的版本 |
| 处理 2 | 试出原构建选项为 `-g`，用 `gcc -g -Wall -Wextra -std=c11 Btree.c -o Btree.exe` 还原（72680 字节，与原值一致） |
| 教训 | **编译产物取名前先 `Get-ChildItem` 看看有没有同名（大小写不同也算同名）的文件**；不要「顺手」重新编译用户已有的产物 |

**调试方法留档**（这次靠它找到 #4）：把源码复制到 `%TEMP%` 加探针，逐帧打印「子树关键字总数 / 每个孩子的大小 / 借位与合并前后结构」，用 `cmd /c "prog > log.txt 2>&1"` 重定向（**不要用 PowerShell 的 `>`**，否则日志会变成 UTF-16 而截断）。

---

### 10. 重新生成输出记录 + 清空编译产物

| 项 | 内容 |
|---|---|
| 操作 1 | 先用 `cmd /c "Btree.exe > Btree_demo_output.txt"` **覆盖**那份损坏的记录（必须先做，否则删完 exe 就没得跑了） |
| 验证 1 | 4261 字节、**无 BOM**（首字节 `3D` 即 `=`）、158 行、**含非 ASCII 字符的行数 = 0** —— 与当初 `Btree.c` 的验证指标完全一致 |
| 操作 2 | 删除全部 6 个编译产物：`Btree.exe`、`Btree_demo.exe`、`bst_demo.exe`、`rbt_demo.exe`、`avl_demo.exe`、`Bplus_demo.exe` |
| 结果 | 工作区只剩源码、`.bak` 备份、`Btree_demo_output.txt`、`shallow_copy.md`、`WORKLOG.md` |

**需要时照抄的重新编译命令：**

| 源文件 | 命令 | 产物 |
|---|---|---|
| `bst.c` | `gcc -Wall -Wextra -std=c11 bst.c -o bst_demo.exe` | `bst_demo.exe` |
| `Btree.c` | `gcc -g -Wall -Wextra -std=c11 Btree.c -o Btree.exe` | `Btree.exe`（原产物带 `-g`） |
| `Btree.c` | `gcc -Wall -Wextra -std=c11 Btree.c -o Btree_demo.exe` | `Btree_demo.exe` |
| `red-black_tree.c` | `gcc -Wall -Wextra -std=c11 red-black_tree.c -o rbt_demo.exe` | `rbt_demo.exe` |
| `avl_tree.c` | `gcc -Wall -Wextra -std=c11 avl_tree.c -o avl_demo.exe` | `avl_demo.exe` |
| `B+tree.c` | `gcc -Wall -Wextra -std=c11 "B+tree.c" -o Bplus_demo.exe` | `Bplus_demo.exe` |
| 输出记录 | `cmd /c "Btree.exe > Btree_demo_output.txt"` | 纯 ASCII，任何编码下都不乱码 |

⚠️ **不要**把 `B+tree.c` 的产物命名为 `btree_demo.exe` —— Windows 文件名不区分大小写，会覆盖 `Btree_demo.exe`（见第 9 节事故记录）。
⚠️ 生成输出记录**必须用 `cmd` 重定向**，PowerShell 的 `>` 会把内容转成 UTF-16 并可能截断（见 `shallow_copy.md` 同期的编码分析）。

---

### 11. 补全其余 8 个文件（从「只有节点定义」变为完整可运行演示）

| 文件 | 原状 | 补全内容 | 自检 |
|---|---|---|---|
| `binary_tree.c` | 24 行骨架 | 四种遍历 / 节点数 / 高度 / 叶子数 / 镜像反转 / 缩进打印 / 释放 | 8/8 |
| `binary_tree_traversal.c` | 3 个递归遍历，缺 `createNode`、缺 `main` | 补齐建节点；新增**非递归**前/中/后序（显式栈）+ 层序；新增「前序 + 中序 → 重建树」 | 8/8 |
| `child_tree.c` | 实为 `binary_tree.c` 的复制 | 用原定义作**引子**（说明 left/right 只能挂两个孩子）；实现**孩子表示法**（孩子链表） | 7/7 |
| `child_sibling_tree.c` | 只有节点定义 | 孩子兄弟表示法；程序验证 ★树的先根 = 二叉树先序、树的后根 = 二叉树中序★ | 10/10 |
| `parents_tree.c` | 实为「三叉链表」节点 | 用它作**对照实现**；新增**双亲表示法**（数组 + 父下标），两法求深度/路径互相印证 | 10/10 |
| `huffman_tree.c` | 只有节点定义 | 贪心建树 / 编码表 / **前缀性检查** / 编解码往返 / WPL 两种算法互证 | 8/8，WPL = 271（与教材一致） |
| `trie_tree.c` | 只有节点定义 | 插入 / 精确查询 / 前缀查询 / 前缀补全 / 删除并剪枝 | 16/16 |
| `segment_tree.c` | 只有节点定义 | 建树 / 区间查询 / 单点修改；**2000 次随机操作与暴力法逐次对拍** | 5/5 + 对拍 2000/2000 |

**统一风格**：文件头大注释块（原理 + 与相邻表示法的对比 + 文件结构 + 约定）；注释中文、输出英文；每个演示都有 `expect` 风格自检并打印 `N passed, M failed`。

**对原有代码的处理：**

| 项 | 说明 |
|---|---|
| 保留 | 8 个文件的 `Node` / `createNode` 定义**逐字保留**。其中 `child_tree.c`、`parents_tree.c` 的原定义与文件名不符（一个是二叉节点、一个是三叉链表），没有删除，而是分别用作「引子」和「对照实现」 |
| 唯一实质改动 | `binary_tree_traversal.c` 三个递归遍历里的 `printf(...)` 改成 `visit(...)`，让「递归版 vs 非递归版」能被程序自动比对（遍历逻辑一行未变） |
| 补强 | 8 个 `createNode` 补上 `malloc` 失败检查（与 `bst.c` 等一致） |
| 备份 | 8 个 `.bak` 已用**原始字节数**逐个校验，与原件**逐字节一致** |

**验证**：13 个源文件全部 `gcc -Wall -Wextra -std=c11` **零警告**；8 个演示合计 **72 项自检全部通过**，另有线段树 2000 次对拍 0 失配。

**一个有用的观察**：8 个文件里最值得用程序验证的两条结论 ——
① 孩子兄弟表示法下「树的先根 = 二叉树先序、树的后根 = 二叉树中序」；
② 线段树每次修改后都必须把祖先的 `sum` 上拉（漏掉不会崩，只会静静给出旧值）。
这两条现在都由程序断言，而不是靠注释声称。

---

### 12. 统一 13 个演示的自检汇总（`bst.c` / `Btree.c`）

| 项 | 内容 |
|---|---|
| 操作 | 给 `bst.c` 与 `Btree.c` 补上 `expect` 式自检与 `result: N passed, M failed` 汇总行（此前只有这 2 个文件没有） |
| `bst.c` 新增 | `expectInt` / `expectSorted`（另写中序收集版，原 `inorder` 只负责打印）+ 13 项断言：建树后节点数 / 高度 / 最值 / 有序性；删除叶子、单孩子、双孩子后的节点数；删除不存在的键；[4] 段退化树的节点数与高度 |
| `Btree.c` 新增 | `recordCheck` / `expectInt` / `expectBool` / `inorderAscending`；`showState` 里的性质校验现在计入总数；另加 15 项断言（插入后 10 键、高度 3、全部可查、缺键查不到；删除后剩 5 键、高度 2、被删的键查不到、保留的键还在） |
| 验证 | 两文件编译零警告；`bst.c` **13 passed / 0 failed**；`Btree.c` **31 passed / 0 failed**（含每步性质校验 16 次） |
| 同步 | 因 `Btree.c` 输出变了，用 `cmd` 重定向重新生成 `Btree_demo_output.txt`：5145 字节、**无 BOM**、179 行、**非 ASCII 行数 0** |

**结果**：13 个源文件风格完全统一 —— 「文件头原理注释 + 中文注释 + 英文输出 + expect 式自检 + `N passed, M failed` 汇总」，全仓库 `gcc -Wall -Wextra -std=c11` 零警告。

---

## 待办 / 遗留

- [ ] 全部 13 个源文件均已完成（可运行 + 自带校验 + 统一风格），暂无待办
- [ ] 若要在工程中使用，重建编译产物的命令见第 10 节（产物当前已清空，工作区只有源码、`.bak`、`Btree_demo_output.txt`、`shallow_copy.md`、`WORKLOG.md`）
