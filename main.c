#include <stdio.h>
#include <string.h>
#include "heap_logic.h"

// 實作 Callback：將狀態序列化為 JSON，並處理阻塞
void send_state_and_block(VisualState* state) {
    printf("{\"heap\": [");
    for(int i = 0; i < state->size; i++) {
        printf("%d%s", state->heap[i], (i < state->size - 1) ? ", " : "");
    }
    
    printf("], \"event\": \"%s\", \"targets\": [", state->event);
    bool has_t1 = (state->target_1 != -1);
    bool has_t2 = (state->target_2 != -1);
    if (has_t1) printf("%d", state->target_1);
    if (has_t1 && has_t2) printf(", ");
    if (has_t2) printf("%d", state->target_2);
    
    printf("], \"is_idle\": %s}\n", state->is_idle ? "true" : "false");
    fflush(stdout); // 確保推向 Python

    // 若演算法尚未結束，卡住等待 Python 的 '\n'
    if (!state->is_idle) {
        char buffer[10];
        fgets(buffer, sizeof(buffer), stdin);
    }
}

int main() {
    int heap[100] = {50, 40, 30, 20}; // 預設資料
    int size = 4;

    while (true) {
        // 發送閒置狀態，讓 Python 解鎖控制面板
        VisualState idle_state = {heap, size, "IDLE", -1, -1, true};
        send_state_and_block(&idle_state);

        // 讀取 Python 的全域指令
        char cmd[50];
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;

        if (strncmp(cmd, "INSERT", 6) == 0) {
            int val;
            if (sscanf(cmd, "INSERT %d", &val) == 1) {
                // 將演算法與通訊函數結合
                heap_insert(heap, &size, val, send_state_and_block);
            }
        }
        else if (strncmp(cmd, "REMOVE", 6) == 0) {
            if (size > 0) {
                heap_remove_top(heap, &size, send_state_and_block);
            } else {
                // 如果 Heap 是空的，發送一個 DONE 狀態讓 Python 回到 IDLE
                VisualState empty_state = {heap, size, "EMPTY", -1, -1, true};
                send_state_and_block(&empty_state);
            }
        }
    }
    return 0;
}
