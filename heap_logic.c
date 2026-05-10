#include "heap_logic.h"

#include "heap_logic.h"

void sift_down(int* heap, int size, int index, StateCallback callback) {
    int root = index;
    while (root * 2 + 1 < size) {
        int child = root * 2 + 1;
        int target = root;

        // COMPARE event
        VisualState state = {heap, size, "COMPARE", target, child, false};
        
        if (heap[target] < heap[child]) target = child;
        if (child + 1 < size) {
            state.target_2 = child + 1;
            callback(&state); // Visualize comparison with second child
            if (heap[target] < heap[child + 1]) target = child + 1;
        } else {
            callback(&state);
        }

        if (target == root) break;
        
        // SWAP event
        int temp = heap[root];
        heap[root] = heap[target];
        heap[target] = temp;
        
        state.event = "SWAP";
        state.target_1 = root;
        state.target_2 = target;
        callback(&state);

        root = target;
    }
}

void heap_sort(int* heap, int* size, StateCallback callback) {
    int n = *size;
    // Build heap (re-arrange array)
    for (int i = n / 2 - 1; i >= 0; i--) {
        sift_down(heap, n, i, callback);
    }

    // One by one extract an element from heap
    for (int i = n - 1; i > 0; i--) {
        // Move current root to end
        int temp = heap[0];
        heap[0] = heap[i];
        heap[i] = temp;

        VisualState state = {heap, n, "SORT_EXTRACT", 0, i, false};
        callback(&state);

        // call max heapify on the reduced heap
        sift_down(heap, i, 0, callback);
    }
    
    VisualState done = {heap, n, "DONE", -1, -1, true};
    callback(&done);
}


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
