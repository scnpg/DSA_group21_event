#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

#define MAX_SIZE 100
//用array實作heap
//未來擴充：可以把 int data 改成 struct Node { int key; char task_name[20]; } 來做priority

/*typedef struct {
    int priority;
    char task[50];
} Task;*/


typedef struct heap{
    int data[MAX_SIZE];
    int size;
    //這次操作的數據 (例如：這次 Insert 換了幾次)
    int cur_swap_count; //交換次數
    int cur_compare_count; //比較次數
    // 整個程式運行至今的累計數據 (用於計算總複雜度)
    int total_swap_count;
    int total_compare_count;
    bool is_max_heap;
}Heap;

void swap(Heap* h, int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
    h->cur_swap_count++;
    h->total_swap_count++;
}

bool compare(Heap* h, int parent_val, int child_val){
    h->cur_compare_count++;
    h->total_compare_count++;
    if(h->is_max_heap){
        return child_val > parent_val;
    }
    else{
        return child_val < parent_val;
    }
}

//todo
void init_heap(Heap* h, bool is_max){ // 初始化 size = 0 與設定 max/min 模式
    h->is_max_heap = is_max;
    h->size = 0;
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
    h->total_compare_count = 0;
    h->total_swap_count = 0;
} 
void reset_stats(Heap* h){// 歸零統計數據，每次insert、extract操作前呼叫
    h->cur_compare_count = 0;
    h->cur_swap_count = 0;
}            
void print_heap_state(Heap* h){// 把當前陣列狀態印出來給 Python 讀取
    printf("STATUS: Cur_Swap: %d, Cur_Compare: %d, Total_Swap: %d, Total_compare: %d\n",h->cur_swap_count,h->cur_compare_count,h->total_swap_count,h->total_compare_count);
    printf("SIZE: %d\n",h->size);
    printf("DATA: ");
    for(int i=0; i<h->size; i++){
        printf("%d ",h->data[i]);
    }
    printf("\n------------\n");
}       

bool is_empty(Heap* h){// 前端可以用來決定是否要把 Extract 按鈕反灰
    return h->size == 0;
} 
bool is_full(Heap* h){// 前端可以用來決定是否要把 Insert 按鈕反灰
    return h->size == MAX_SIZE;
} 
bool is_valid_heap(Heap* h); // 檢查當前陣列是否真的符合 Heap 規則 (除錯用)

void sift_up(Heap* h, int index){//往上移動的過程 比較並交換，直到遇到比自己大(Max)或小(Min)的父節點
    while(index > 0){
        int parent = (index-1)/2;
        printf("ACTION: COMPARE ( %d , %d )\n",index,parent);
        if(compare(h, h->data[parent], h->data[index])){
            
            printf("ACTION: SWAP ( %d , %d )\n",index,parent);
            swap(h, &(h->data[index]), &(h->data[parent]));

            index = parent;
        }
        else{
            break;
        }
    }
} 
void sift_down(Heap* h, int index){//往下移動的過程 比較並交換，直到遇到比自己小(Max)或大(Min)的子節點
    int left = 2*index+1;
    while(left < h->size){
        int right = 2*index+2;
        int extreme_child = left;
        if(right < h->size){
            printf("ACTION: COMPARE ( %d , %d )\n",left,right);
            if(compare(h,h->data[left],h->data[right])){
                extreme_child = right;
            }
        }

        printf("ACTION: COMPARE ( %d , %d )\n",index,extreme_child);
        if(compare(h,h->data[index],h->data[extreme_child])){
            printf("ACTION: SWAP ( %d , %d )\n",index,extreme_child);
            swap(h, &(h->data[index]), &(h->data[extreme_child]));

            index = extreme_child;
            left = 2*index+1;
        }
        else{
            break;
        }
    }
}


void insert(Heap* h, int value){//插入在尾端 將值放在陣列尾端，size + 1，然後呼叫 sift_up 讓它浮動到正確位置
    if(is_full(h)){
        printf("ERROR: Heap is full!\n");
        return;
    }
    
    reset_stats(h);

    int index = h->size;
    h->data[index] = value;
    h->size++;
    printf("ACTION: INSERT %d AT INDEX %d\n", value, index);

    sift_up(h,index);
}
int extract_top(Heap* h){//pop 記錄頂端值準備回傳，將尾端元素移到頂端 (index 0)，size - 1，然後呼叫 sift_down 讓它沉澱
    if(is_empty(h)){
        printf("ERROR: Heap is empty!\n");
        return -1;
    }

    reset_stats(h);

    int top = h->data[0];
    h->data[0] = h->data[h->size-1];
    h->size--;
    printf("ACTION: EXTRACT %d\n",top);
    sift_down(h,0);
    return top;
}
int peek_top(Heap* h);
int get_height(Heap* h); //計算這棵樹目前有幾層
int get_level(int index);//回傳某個 index 位於樹的第幾層

void build_heap(Heap* h, int *arr, int n){//從雜亂的陣列建一個maxheap或是minheap
    h->size = 0;
    for(int i = 0; i < n && i < MAX_SIZE; i++){
        h->data[i] = arr[i];
        h->size++;
    }
    printf("ACTION: BUILD_HEAP_START\n");
    reset_stats(h);

    for(int i=(h->size/2)-1; i >= 0; i--){
        printf("ACTION: BUILD_STEP AT_INDEX %d\n", i);
        sift_down(h, i);
    }
    printf("ACTION: BUILD_HEAP_DONE. Total_Compare: %d\n", h->total_compare_count);
}
void heap_sort(Heap* h);//heap sort 不斷 extract_top 將陣列由小到大或由大到小排序

void update_key(Heap *h, int index, int new_value){// 更改某個節點的值，並重新 Sift 調整
    if (index < 0 || index >= h->size){
        printf("ERROR: Invalid index %d\n", index);
        return;
    }
    reset_stats(h);
    int old_value = h->data[index];
    h->data[index] = new_value;
    printf("ACTION: UPDATE_KEY INDEX:%d FROM:%d TO:%d\n", index, old_value, new_value);
    
    if (index == 0) {
        sift_down(h, 0);
    } else {
        int parent = (index-1)/2;
        printf("ACTION: COMPARE_FOR_DIRECTION ( %d , %d )\n", index, parent);
        if (compare(h, h->data[parent], h->data[index])){
            sift_up(h, index);
        }
        else{
            sift_down(h, index);
        }
    }
} 
void delete_idx(Heap *h, int index){// 刪除任意 index 的節點，與尾端交換後重新 Sift 調整
    if (index < 0 || index >= h->size){
        printf("ERROR: Invalid index %d\n", index);
        return;
    }
    
    reset_stats(h);
    int deleted_value = h->data[index];

    if (index == h->size-1){
        printf("ACTION: DELETE_LAST INDEX:%d VALUE:%d\n", index, deleted_value);
        h->size--;
    }
    else{
        int last_value = h->data[h->size-1]; //將最後一個位置的人搬到index位置
        h->size--;
        printf("ACTION: DELETE_REPLACE INDEX:%d WITH_VALUE:%d (ORIGINAL_WAS:%d)\n", index, last_value, deleted_value);
        update_key(h, index, last_value);
    }
}
void invert_heap(Heap* h);                  // Max-Heap 與 Min-Heap 瞬間切換
void merge_heaps(Heap* h1, Heap* h2, Heap* result); // 雙堆合併動畫
int search_value(Heap* h, int target);      // 展現 O(N) 搜尋的動畫
int find_kth(Heap* h, int k);               // 尋找第 K 大/小元素
void batch_insert(Heap* h, int *arr, int n);// 批次匯入展示 O(N log N) 效能
void clear_heap(Heap* h);                   // 一鍵清空


void print_prefix(Heap* h);
void print_infix(Heap* h);
void print_suffix(Heap* h);
//目前的想法 歡迎多加補充

int main(){
    //讀python的指令
    //例如讀到inset(10)就執行insert function
    Heap newheap;
    int a;
    scanf("%d",&a);//0為min 1為max
    init_heap(&newheap, a>0);
    
    char command[50];
    int value;
    while(scanf("%s",command)!= EOF){
        if(strcmp(command, "INSERT") == 0){
            scanf("%d",&value);
            insert(&newheap, value);
            print_heap_state(&newheap);
        }
        else if(strcmp(command, "EXTRACT") == 0){
            int ext = extract_top(&newheap);
            printf("RESULT_EXTRACT: %d\n", ext);
            print_heap_state(&newheap);
        }
        else if (strcmp(command, "EXIT") == 0) {
            break;
        }
    }
    return 0;
}