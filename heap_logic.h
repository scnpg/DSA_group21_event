#ifndef HEAP_LOGIC_H
#define HEAP_LOGIC_H

#include <stdbool.h>

typedef struct {
    int* heap;
    int size;
    const char* event;
    int target_1;
    int target_2;
    bool is_idle;
} VisualState;

typedef void (*StateCallback)(VisualState* state);

void heap_insert(int* heap, int* size, int value, StateCallback callback);
void sift_down(int* heap, int size, int index, StateCallback callback);
void heap_sort(int* heap, int* size, StateCallback callback);
void heap_remove_top(int* heap, int* size, StateCallback callback);

#endif
