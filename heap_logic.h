#ifndef HEAP_LOGIC_H
#define HEAP_LOGIC_H

#include <stdbool.h>

#define MAX_SIZE 100

// 升級：使用 Heap 結構體來儲存狀態與統計數據
typedef struct {
    int data[MAX_SIZE];
    int size;
    int cur_swap_count;
    int cur_compare_count;
    int total_swap_count;
    int total_compare_count;
    bool is_max_heap;
} Heap;

// 定義回呼狀態
typedef struct {
    Heap* h;
    const char* event;
    int targets[MAX_SIZE];  // 目標節點索引陣列
    int num_targets;        // 實際目標數量
    bool is_idle;
    int sort_boundary;  // heap sort 用：index >= sort_boundary 表示「已排序、固定」。-1 表示沒有已排序區
} VisualState;

typedef void (*StateCallback)(VisualState* state);

// 核心函式
void init_heap(Heap* h, bool is_max);
void reset_stats(Heap* h);
bool is_empty(Heap* h);
bool is_full(Heap* h);

// 演算法操作
void heap_insert(Heap* h, int value, StateCallback callback);
int heap_extract_top(Heap* h, StateCallback callback);
void sift_down(Heap* h, int index, StateCallback callback);
void sift_up(Heap* h, int index, StateCallback callback);
void build_heap(Heap* h, int *arr, int n, StateCallback callback);
void heap_sort(Heap* h, StateCallback callback);
void update_key(Heap* h, int index, int new_value, StateCallback callback);
void delete_idx(Heap* h, int index, StateCallback callback);
void invert_heap(Heap* h, StateCallback callback);
void clear_heap(Heap* h, StateCallback callback);
int search_value(Heap* h, int target, StateCallback callback);

// 遍歷輸出 (列印至 stderr 以免污染 JSON 管線)
void print_prefix(Heap* h);
void print_infix(Heap* h);
void print_suffix(Heap* h);

#endif