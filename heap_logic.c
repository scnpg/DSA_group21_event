#include "heap_logic.h"
#include <stdio.h>
#include <math.h>

// 內部小工具：打包並觸發狀態（一般操作，sort_boundary 預設 -1）
static void trigger_state(Heap* h, const char* event, int t1, int t2, bool is_idle, StateCallback callback) {
    VisualState state;
    state.h = h;
    state.event = event;
    state.is_idle = is_idle;
    state.sort_boundary = -1;
    state.num_targets = 0;
    if (t1 != -1) state.targets[state.num_targets++] = t1;
    if (t2 != -1) state.targets[state.num_targets++] = t2;
    callback(&state);
}

// 給 heap_sort 用：帶 sort_boundary
static void trigger_state_with_bound(Heap* h, const char* event, int t1, int t2, bool is_idle, int bound, StateCallback callback) {
    VisualState state;
    state.h = h;
    state.event = event;
    state.is_idle = is_idle;
    state.sort_boundary = bound;
    state.num_targets = 0;
    if (t1 != -1) state.targets[state.num_targets++] = t1;
    if (t2 != -1) state.targets[state.num_targets++] = t2;
    callback(&state);
}

// 給 search 用：傳入任意數量的目標索引
static void trigger_state_multi(Heap* h, const char* event, int* targets, int num_targets, bool is_idle, StateCallback callback) {
    VisualState state;
    state.h = h;
    state.event = event;
    state.is_idle = is_idle;
    state.sort_boundary = -1;
    state.num_targets = num_targets < MAX_SIZE ? num_targets : MAX_SIZE;
    for (int i = 0; i < state.num_targets; i++) state.targets[i] = targets[i];
    callback(&state);
}

void swap(Heap* h, int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
    h->cur_swap_count++;
    h->total_swap_count++;
}

bool compare(Heap* h, int parent_val, int child_val) {
    h->cur_compare_count++;
    h->total_compare_count++;
    return h->is_max_heap ? (child_val > parent_val) : (child_val < parent_val);
}

void init_heap(Heap* h, bool is_max) {
    h->is_max_heap = is_max;
    h->size = 0;
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
    h->total_compare_count = 0;
    h->total_swap_count = 0;
}

void reset_stats(Heap* h) {
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
}

bool is_empty(Heap* h) { return h->size == 0; }
bool is_full(Heap* h) { return h->size == MAX_SIZE; }

void sift_up(Heap* h, int index, StateCallback callback) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        trigger_state(h, "COMPARE", index, parent, false, callback);
        
        if (compare(h, h->data[parent], h->data[index])) {
            swap(h, &(h->data[index]), &(h->data[parent]));
            trigger_state(h, "SWAP", index, parent, false, callback);
            index = parent;
        } else {
            break;
        }
    }
}

// 真正的 sift_down 實作：bound 控制邊界
// - 一般操作呼叫 sift_down → bound = h->size，sort_boundary=-1（無已排序區）
// - heap_sort 呼叫 → bound = 縮小中的 boundary，sort_boundary=bound（顯示已排序區）
static void _sift_down(Heap* h, int index, int bound, StateCallback callback) {
    int sort_bound = (bound == h->size) ? -1 : bound;
    int left = 2 * index + 1;
    while (left < bound) {
        int right = 2 * index + 2;
        int extreme_child = left;

        if (right < bound) {
            if (compare(h, h->data[left], h->data[right])) {
                extreme_child = right;
            }
        }

        trigger_state_with_bound(h, "COMPARE", index, extreme_child, false, sort_bound, callback);

        if (compare(h, h->data[index], h->data[extreme_child])) {
            swap(h, &(h->data[index]), &(h->data[extreme_child]));
            trigger_state_with_bound(h, "SWAP", index, extreme_child, false, sort_bound, callback);
            index = extreme_child;
            left = 2 * index + 1;
        } else {
            break;
        }
    }
}

void sift_down(Heap* h, int index, StateCallback callback) {
    _sift_down(h, index, h->size, callback);
}

void heap_insert(Heap* h, int value, StateCallback callback) {
    if (is_full(h)) {
        fprintf(stderr, "ERROR: Heap is full!\n");
        return;
    }
    
    reset_stats(h);
    int index = h->size;
    h->data[index] = value;
    h->size++;
    
    trigger_state(h, "INSERTED", index, -1, false, callback);
    sift_up(h, index, callback);
    // 修正：DONE 必須是 false，讓前端按最後一次下一步
    trigger_state(h, "DONE", -1, -1, false, callback);
}

int heap_extract_top(Heap* h, StateCallback callback) {
    if (is_empty(h)) {
        fprintf(stderr, "ERROR: Heap is empty!\n");
        return -1;
    }

    reset_stats(h);
    trigger_state(h, "EXTRACT_PREPARE", 0, h->size - 1, false, callback);

    int top = h->data[0];
    h->data[0] = h->data[h->size - 1];
    trigger_state(h, "EXTRACT_SWAP", 0, h->size - 1, false, callback);
    
    h->size--;
    
    if (h->size > 0) {
        trigger_state(h, "REMOVED_START_SIFT", -1, -1, false, callback);
        sift_down(h, 0, callback);
    }
    
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
    return top;
}

void build_heap(Heap* h, int *arr, int n, StateCallback callback) {
    h->size = 0;
    for (int i = 0; i < n && i < MAX_SIZE; i++) {
        h->data[i] = arr[i];
        h->size++;
    }
    reset_stats(h);
    
    trigger_state(h, "INSERTED", 0, h->size - 1, false, callback);

    for (int i = (h->size / 2) - 1; i >= 0; i--) {
        sift_down(h, i, callback);
    }
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
}

void heap_sort(Heap* h, StateCallback callback) {
    // size <= 1 不用排
    if (h->size <= 1) {
        trigger_state(h, "DONE", -1, -1, false, callback);
        return;
    }

    reset_stats(h);

    // 先備份原始 heap，排完之後要還原
    int original_data[MAX_SIZE];
    int original_size = h->size;
    for (int i = 0; i < original_size; i++) {
        original_data[i] = h->data[i];
    }

    // 反覆把 root（max）換到 heap 區尾端，boundary 縮小一格
    // 注意：呼叫 SORT 前 heap 已經是合法 max heap，不需要再 heapify 一次
    // 注意：boundary 是「還在 heap 區的元素數量」，h->size 保持 original_size 不變
    //       這樣前端會一直看到全部元素，包含已排序到後段的最大值
    int boundary = original_size;
    for (int i = 0; i < original_size - 1; i++) {
        int last_idx = boundary - 1;
        // 先 swap，再把 boundary 縮小一格，讓 SWAP 這一幀就能看到正確的已排序區
        swap(h, &(h->data[0]), &(h->data[last_idx]));
        boundary--;
        trigger_state_with_bound(h, "SWAP", 0, last_idx, false, boundary, callback);
        _sift_down(h, 0, boundary, callback);
    }

    // 顯示排好的升冪結果（boundary=0 表示整個陣列都是已排序區）
    trigger_state_with_bound(h, "DONE", -1, -1, false, 0, callback);

    // 靜默還原回 sort 前的 heap（不 trigger，不阻塞）
    for (int i = 0; i < original_size; i++) {
        h->data[i] = original_data[i];
    }
    // ← 第二個 DONE 已移除（Option A：靜默還原）
}

void update_key(Heap *h, int index, int new_value, StateCallback callback) {
    if (index < 0 || index >= h->size) {
        fprintf(stderr, "ERROR: Invalid index %d\n", index);
        return;
    }
    reset_stats(h);
    h->data[index] = new_value;
    
    trigger_state(h, "INSERTED", index, -1, false, callback);
    
    if (index == 0) {
        sift_down(h, 0, callback);
    } else {
        int parent = (index - 1) / 2;
        trigger_state(h, "COMPARE", index, parent, false, callback);
        if (compare(h, h->data[parent], h->data[index])) {
            sift_up(h, index, callback);
        } else {
            sift_down(h, index, callback);
        }
    }
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
}

void delete_idx(Heap *h, int index, StateCallback callback) {
    if (index < 0 || index >= h->size) {
        fprintf(stderr, "ERROR: Invalid index %d\n", index);
        return;
    }
    reset_stats(h);

    if (index == h->size - 1) {
        h->size--;
        trigger_state(h, "DONE", -1, -1, false, callback);
    } else {
        trigger_state(h, "DELETE_PREPARE", index, h->size - 1, false, callback);
        int last_value = h->data[h->size - 1];
        h->data[index] = last_value;
        h->size--;
        trigger_state(h, "DELETE_SWAP", index, h->size, false, callback);
        
        int parent = (index - 1) / 2;
        if (index > 0) trigger_state(h, "COMPARE", index, parent, false, callback);
        
        if (index > 0 && compare(h, h->data[parent], h->data[index])) sift_up(h, index, callback);
        else sift_down(h, index, callback);
        
        trigger_state(h, "DONE", -1, -1, false, callback);
    }
}

void invert_heap(Heap* h, StateCallback callback) {
    h->is_max_heap = !h->is_max_heap;
    reset_stats(h);
    trigger_state(h, "INVERT_START", -1, -1, false, callback);
    for (int i = (h->size / 2) - 1; i >= 0; i--) {
        sift_down(h, i, callback);
    }
    trigger_state(h, "DONE", -1, -1, false, callback);
}

int search_value(Heap* h, int target, StateCallback callback) {
    reset_stats(h);
    int found[MAX_SIZE];
    int num_found = 0;
    int cmp_targets[MAX_SIZE + 1];

    for (int i = 0; i < h->size; i++) {
        // 手動計入這次比較（search 用 == 不走 compare()，需自行累計）
        h->cur_compare_count++;
        h->total_compare_count++;

        // SEARCH_CMP: targets[0] = 當前比較節點，targets[1..] = 已找到的節點
        int cmp_count = 0;
        cmp_targets[cmp_count++] = i;
        for (int j = 0; j < num_found; j++) cmp_targets[cmp_count++] = found[j];
        trigger_state_multi(h, "SEARCH_CMP", cmp_targets, cmp_count, false, callback);

        if (h->data[i] == target) {
            found[num_found++] = i;
            // FOUND: targets = 所有已找到的節點（含剛找到的）
            trigger_state_multi(h, "FOUND", found, num_found, false, callback);
        }
    }

    if (num_found > 0) {
        trigger_state_multi(h, "SEARCH_DONE", found, num_found, false, callback);
    } else {
        trigger_state(h, "SEARCH_NOT_FOUND", -1, -1, false, callback);
    }
    return num_found > 0 ? found[0] : -1;
}

void clear_heap(Heap* h, StateCallback callback) {
    h->size = 0;
    reset_stats(h);
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
}

// === 遍歷列印 (走 stderr 避免干擾 JSON IPC) ===
void _prefix(Heap* h, int i) {
    if (i >= h->size) return;
    fprintf(stderr, "%d ", h->data[i]);
    _prefix(h, 2 * i + 1);
    _prefix(h, 2 * i + 2);
}
void print_prefix(Heap* h) { fprintf(stderr, "PREFIX: "); _prefix(h, 0); fprintf(stderr, "\n"); }

void _infix(Heap* h, int i) {
    if (i >= h->size) return;
    _infix(h, 2 * i + 1);
    fprintf(stderr, "%d ", h->data[i]);
    _infix(h, 2 * i + 2);
}
void print_infix(Heap* h) { fprintf(stderr, "INFIX: "); _infix(h, 0); fprintf(stderr, "\n"); }

void _suffix(Heap* h, int i) {
    if (i >= h->size) return;
    _suffix(h, 2 * i + 1);
    _suffix(h, 2 * i + 2);
    fprintf(stderr, "%d ", h->data[i]);
}
void print_suffix(Heap* h) { fprintf(stderr, "SUFFIX: "); _suffix(h, 0); fprintf(stderr, "\n"); }