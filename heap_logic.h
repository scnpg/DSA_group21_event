#ifndef HEAP_LOGIC_H
#define HEAP_LOGIC_H

#include <stdbool.h>

// 對應 Python dataclass 的 C 結構體
typedef struct {
    int* heap;
    int size;
    const char* event;
    int target_1;
    int target_2;
    bool is_idle;
} VisualState;

// 定義 Callback 函數指標 (讓 C1 可以把狀態傳給 C2)
typedef void (*StateCallback)(VisualState* state);

void heap_insert(int* heap, int* size, int value, StateCallback callback);

#endif