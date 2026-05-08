#include "heap_logic.h"

void heap_insert(int* heap, int* size, int value, StateCallback callback) {
    int i = *size;
    heap[i] = value;
    (*size)++;

    // 狀態 1：剛放入陣列尾端
    VisualState state = {heap, *size, "INSERTED", i, -1, false};
    callback(&state);

    // 開始向上篩選 (Sift Up)
    while (i > 0) {
        int parent = (i - 1) / 2;
        
        // 狀態 2：比較中
        state.event = "COMPARE";
        state.target_1 = i;
        state.target_2 = parent;
        callback(&state);

        if (heap[parent] < heap[i]) {
            // 交換邏輯
            int temp = heap[parent];
            heap[parent] = heap[i];
            heap[i] = temp;

            // 狀態 3：發生交換
            state.event = "SWAP";
            callback(&state);
            i = parent;
        } else {
            break;
        }
    }
    
    // 狀態 4：操作完成，回歸閒置
    state.event = "DONE";
    state.target_1 = -1;
    state.target_2 = -1;
    state.is_idle = true;
    callback(&state);
}