#include "heap_logic.h"
#include <stdio.h>
#include <math.h>

// 內部小工具：打包並觸發狀態
static void trigger_state(Heap* h, const char* event, int t1, int t2, bool is_idle, StateCallback callback) {
    VisualState state = {h, event, t1, t2, is_idle};
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

void sift_down(Heap* h, int index, StateCallback callback) {
    int left = 2 * index + 1;
    while (left < h->size) {
        int right = 2 * index + 2;
        int extreme_child = left;
        
        if (right < h->size) {
            if (compare(h, h->data[left], h->data[right])) {
                extreme_child = right;
            }
        }

        trigger_state(h, "COMPARE", index, extreme_child, false, callback);
        
        if (compare(h, h->data[index], h->data[extreme_child])) {
            swap(h, &(h->data[index]), &(h->data[extreme_child]));
            trigger_state(h, "SWAP", index, extreme_child, false, callback);
            index = extreme_child;
            left = 2 * index + 1;
        } else {
            break;
        }
    }
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
    //建立備份
    int original_data[MAX_SIZE];
    int original_size = h->size;
    for (int i = 0; i < h->size; i++) {
        original_data[i] = h->data[i];
    }

    // 確保目前是合法的 Heap 結構（建構 Max Heap）
    for (int i = (h->size / 2) - 1; i >= 0; i--) {
        sift_down(h, i, callback);
    }
    
    //執行 Heap Sort
    for (int i = 0; i < original_size - 1; i++) {
        int last_idx = h->size - 1;
        trigger_state(h, "EXTRACT_PREPARE", 0, last_idx, false, callback);
        swap(h, &(h->data[0]), &(h->data[last_idx]));
        trigger_state(h, "EXTRACT_SWAP", 0, last_idx, false, callback);
        
        h->size--;
        sift_down(h, 0, callback);
    }
    
    //恢復 size 讓前端能看到完整的已排序陣列
    h->size = original_size; 
    
    //觸發一個狀態讓畫面停留在「排序完成」，等待按下「下一步」
    trigger_state(h, "SORT_COMPLETED", -1, -1, false, callback);

    //按下「下一步」後將備份的資料寫回，完全還原原本的 Heap 結構
    for (int i = 0; i < h->size; i++) {
        h->data[i] = original_data[i];
    }

    //顯示還原後的 Heap 畫面
    trigger_state(h, "DONE", -1, -1, false, callback);
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
        trigger_state(h, "EXTRACT_PREPARE", index, h->size - 1, false, callback);
        int last_value = h->data[h->size - 1];
        h->data[index] = last_value;
        h->size--;
        trigger_state(h, "EXTRACT_SWAP", index, h->size, false, callback);
        
        int parent = (index - 1) / 2;
        if (index > 0) trigger_state(h, "COMPARE", index, parent, false, callback);
        
        if (index > 0 && compare(h, h->data[parent], h->data[index])) sift_up(h, index, callback);
        else sift_down(h, index, callback);
        
        // 修正：DONE 必須是 false
        trigger_state(h, "DONE", -1, -1, false, callback);
    }
}

void invert_heap(Heap* h, StateCallback callback) {
    h->is_max_heap = !h->is_max_heap;
    trigger_state(h, "REMOVED_START_SIFT", -1, -1, false, callback);
    for (int i = (h->size / 2) - 1; i >= 0; i--) {
        sift_down(h, i, callback);
    }
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
}

int search_value(Heap* h, int target, StateCallback callback) {
    for (int i = 0; i < h->size; i++) {
        trigger_state(h, "COMPARE", i, -1, false, callback); 
        if (h->data[i] == target) {
            trigger_state(h, "SWAP", i, -1, false, callback); 
            // 修正：DONE 必須是 false
            trigger_state(h, "DONE", -1, -1, false, callback);
            return i;
        }
    }
    // 修正：DONE 必須是 false
    trigger_state(h, "DONE", -1, -1, false, callback);
    return -1;
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