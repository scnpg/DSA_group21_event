#include <stdio.h>
#include <string.h>
#include "heap_logic.h"

// ================= IPC 通訊與動畫阻塞 =================

void notify_frontend(Heap* h, const char* event, int target1, int target2, bool is_idle) {
    // 1. 輸出 JSON 格式的陣列
    printf("{\"heap\": [");
    for(int i = 0; i < h->size; i++) {
        printf("%d%s", h->data[i], (i < h->size - 1) ? ", " : "");
    }
    
    // 2. 輸出事件與目標索引
    printf("], \"event\": \"%s\", \"targets\": [", event);
    if (target1 != -1 && target2 != -1) {
        printf("%d, %d", target1, target2);
    } else if (target1 != -1) {
        printf("%d", target1);
    }
    
    // 3. 輸出閒置狀態與統計數據
    printf("], \"is_idle\": %s, ", is_idle ? "true" : "false");
    printf("\"stats\": {\"cur_swap\": %d, \"cur_cmp\": %d, \"tot_swap\": %d, \"tot_cmp\": %d}}\n",
           h->cur_swap_count, h->cur_compare_count, h->total_swap_count, h->total_compare_count);
           
    // 4. 強制推播至 Python 管線
    fflush(stdout); 
    
    // 5. 動畫阻塞 (Blocking)：如果不是閒置狀態，就卡住等待前端按下「下一步」
    if (!is_idle) {
        char step_buffer[10];
        fgets(step_buffer, sizeof(step_buffer), stdin);
    }
}

// ================= 基礎工具函式 =================

void init_heap(Heap* h, bool is_max) {
    h->is_max_heap = is_max;
    h->size = 0;
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
    h->total_compare_count = 0;
    h->total_swap_count = 0;
    // 告訴前端系統已重置並進入閒置狀態
    notify_frontend(h, "INIT", -1, -1, true); 
}

void reset_stats(Heap* h) {
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
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
    if(h->is_max_heap) return child_val > parent_val;
    else return child_val < parent_val;
}

// ================= 核心調整函式 =================

void sift_up(Heap* h, int index) {
    while(index > 0) {
        int parent = (index - 1) / 2;
        
        // 動畫影格：發送 COMPARE 事件並暫停
        notify_frontend(h, "COMPARE", index, parent, false);
        
        if(compare(h, h->data[parent], h->data[index])) {
            swap(h, &(h->data[index]), &(h->data[parent]));
            
            // 動畫影格：發送 SWAP 事件並暫停
            notify_frontend(h, "SWAP", index, parent, false);
            index = parent;
        } else {
            break;
        }
    }
    // 調整結束，回到閒置狀態，解鎖前端 UI
    notify_frontend(h, "IDLE", -1, -1, true);
}

void sift_down(Heap* h, int index) {
    int left = 2 * index + 1;
    while(left < h->size) {
        int right = 2 * index + 2;
        int extreme_child = left;
        
        if(right < h->size) {
            notify_frontend(h, "COMPARE", left, right, false);
            if(compare(h, h->data[left], h->data[right])) {
                extreme_child = right;
            }
        }

        notify_frontend(h, "COMPARE", index, extreme_child, false);
        if(compare(h, h->data[index], h->data[extreme_child])) {
            swap(h, &(h->data[index]), &(h->data[extreme_child]));
            notify_frontend(h, "SWAP", index, extreme_child, false);
            index = extreme_child;
            left = 2 * index + 1;
        } else {
            break;
        }
    }
    notify_frontend(h, "IDLE", -1, -1, true);
}

// ================= 操作 API =================

void insert(Heap* h, int value) {
    if(h->size >= MAX_SIZE) return;
    reset_stats(h);
    int index = h->size;
    h->data[index] = value;
    h->size++;
    
    // 剛插入時在尾端閃爍一下
    notify_frontend(h, "INSERT", index, -1, false);
    sift_up(h, index);
}

int extract_top(Heap* h) {
    if(h->size == 0) return -1;
    reset_stats(h);
    
    int top = h->data[0];
    // 展示準備移除頂端
    notify_frontend(h, "EXTRACT", 0, -1, false);
    
    h->data[0] = h->data[h->size - 1];
    h->size--;
    
    if(h->size > 0) {
        notify_frontend(h, "SWAP_TO_TOP", 0, -1, false);
        sift_down(h, 0);
    } else {
        notify_frontend(h, "IDLE", -1, -1, true);
    }
    return top;
}