#ifndef HEAP_LOGIC_H
#define HEAP_LOGIC_H

#include <stdbool.h>

#define MAX_SIZE 100

// 定義 Heap 結構
typedef struct heap {
    int data[MAX_SIZE];
    int size;
    int cur_swap_count;
    int cur_compare_count;
    int total_swap_count;
    int total_compare_count;
    bool is_max_heap;
} Heap;

// 核心操作 API
void init_heap(Heap* h, bool is_max);
void reset_stats(Heap* h);

// 演算法操作
void insert(Heap* h, int value);
int extract_top(Heap* h);

// IPC 通訊函式：將狀態打包成 JSON 發送給 Python 並暫停
void notify_frontend(Heap* h, const char* event, int target1, int target2, bool is_idle);

#endif