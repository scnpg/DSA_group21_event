#include <stdio.h>
#include <string.h>
#include "heap_logic.h"

// 將狀態序列化為符合 README_backend.md 規範的 JSON 格式
void send_state_and_block(VisualState* state) {
    Heap* h = state->h;

    // 1. JSON: 陣列狀態
    printf("{\"heap\": [");
    for(int i = 0; i < h->size; i++) {
        printf("%d%s", h->data[i], (i == h->size - 1) ? "" : ", ");
    }
    
    // 2. JSON: 事件與目標陣列
    printf("], \"size\": %d, \"event\": \"%s\", \"targets\": ", h->size, state->event);
    if (state->target_1 != -1 && state->target_2 != -1) {
        printf("[%d, %d]", state->target_1, state->target_2);
    } else if (state->target_1 != -1) {
        printf("[%d]", state->target_1);
    } else {
        printf("[]");
    }

    // 3. JSON: 閒置旗標與儀表板數據
    printf(", \"is_idle\": %s, ", state->is_idle ? "true" : "false");
    printf("\"stats\": {\"cur_swap\": %d, \"cur_cmp\": %d, \"tot_swap\": %d, \"tot_cmp\": %d}}\n",
           h->cur_swap_count, h->cur_compare_count, h->total_swap_count, h->total_compare_count);
           
    fflush(stdout); // 強制推播至 Python 管線

    // 4. 動畫阻塞等待前端按「下一步」(\n)
    if (!state->is_idle) {
        char step_buffer[10];
        fgets(step_buffer, sizeof(step_buffer), stdin);
    }
}

int main() {
    Heap my_heap;
    init_heap(&my_heap, true); // 預設為 Max Heap
    
    // // 初始化一些資料方便直接看到效果
    // int init_arr[] = {50, 40, 30, 20};
    // build_heap(&my_heap, init_arr, 4, send_state_and_block);

    char cmd[150];
    while (true) {
        // 發送全局閒置，解鎖前端操作面板
        VisualState idle_state = {&my_heap, "IDLE", -1, -1, true};
        send_state_and_block(&idle_state);

        // 讀取前端送來的大指令 (只要讀到 EOF 或 EXIT 就乖乖關閉 Process)
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        if (strncmp(cmd, "EXIT", 4) == 0) break;

        if (strncmp(cmd, "INSERT ", 7) == 0) {
            int val;
            if (sscanf(cmd, "INSERT %d", &val) == 1) heap_insert(&my_heap, val, send_state_and_block);
        }
        else if (strncmp(cmd, "EXTRACT", 7) == 0 || strncmp(cmd, "REMOVE", 6) == 0) {
            if (!is_empty(&my_heap)) heap_extract_top(&my_heap, send_state_and_block);
            else {
                VisualState empty = {&my_heap, "EMPTY", -1, -1, true};
                send_state_and_block(&empty);
            }
        }
        else if (strncmp(cmd, "SORT", 4) == 0) {
            heap_sort(&my_heap, send_state_and_block);
        }
        else if (strncmp(cmd, "UPDATE ", 7) == 0) {
            int idx, val;
            if (sscanf(cmd, "UPDATE %d %d", &idx, &val) == 2) update_key(&my_heap, idx, val, send_state_and_block);
        }
        else if (strncmp(cmd, "DELETE ", 7) == 0) {
            int idx;
            if (sscanf(cmd, "DELETE %d", &idx) == 1) delete_idx(&my_heap, idx, send_state_and_block);
        }
        else if (strncmp(cmd, "INIT ", 5) == 0) {
            int arr[MAX_SIZE];
            int n = 0;
            char* p = cmd + 5;
            while (*p && *p != '\n') {
                int val;
                if (sscanf(p, "%d", &val) == 1) {
                    arr[n++] = val;
                    while(*p && *p != ',' && *p != '\n') p++;
                    if(*p == ',') p++;
                } else break;
            }
            build_heap(&my_heap, arr, n, send_state_and_block);
        }
        else if (strncmp(cmd, "INVERT", 6) == 0) {
            invert_heap(&my_heap, send_state_and_block);
        }
        else if (strncmp(cmd, "CLEAR", 5) == 0) {
            clear_heap(&my_heap, send_state_and_block);
        }
        else if (strncmp(cmd, "SEARCH ", 7) == 0) {
            int target;
            if (sscanf(cmd, "SEARCH %d", &target) == 1) search_value(&my_heap, target, send_state_and_block);
        }
    }
    return 0;
}