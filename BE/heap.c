#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include <math.h>

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

void send_state_and_block(Heap* h, const char* event, int target1, int target2, bool is_idle) {
    // 1. 印出陣列狀態
    printf("{\"heap\": [");
    for(int i = 0; i < h->size; i++) {
        printf("%d%s", h->data[i], (i < h->size - 1) ? ", " : "");
    }
    
    // 2. 印出事件與目標
    printf("], \"event\": \"%s\", \"targets\": [", event);
    if (target1 != -1 && target2 != -1) printf("%d, %d", target1, target2);
    else if (target1 != -1) printf("%d", target1);
    
    // 3. 印出 idle 狀態並強制推播 (flush)
    printf("], \"is_idle\": %s}\n", is_idle ? "true" : "false");
    fflush(stdout); 

    // 4. 動畫阻塞機制：如果是動畫過程 (is_idle = false)，就在這邊卡住等前端按下一步
    if (!is_idle) {
        char buffer[10];
        fgets(buffer, sizeof(buffer), stdin);
    }
}

// void print_heap_state(Heap* h){// 把當前陣列狀態印出來給 Python 讀取
//     printf("STATUS: Cur_Swap: %d, Cur_Compare: %d, Total_Swap: %d, Total_compare: %d\n",h->cur_swap_count,h->cur_compare_count,h->total_swap_count,h->total_compare_count);
//     printf("SIZE: %d\n",h->size);
//     printf("DATA: ");
//     for(int i=0; i<h->size; i++){
//         printf("%d ",h->data[i]);
//     }
//     printf("\n------------\n");
// }       

bool is_empty(Heap* h){// 前端可以用來決定是否要把 Extract 按鈕反灰
    return h->size == 0;
} 
bool is_full(Heap* h){// 前端可以用來決定是否要把 Insert 按鈕反灰
    return h->size == MAX_SIZE;
} 
bool is_valid_heap(Heap* h) {// 檢查當前陣列是否真的符合 Heap 規則 (除錯用)
    for (int i = 0; i <= (h->size - 2) / 2; i++) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (h->is_max_heap) {
            if (h->data[i] < h->data[left]) return false;
            if (right < h->size && h->data[i] < h->data[right]) return false;
        } else {
            if (h->data[i] > h->data[left]) return false;
            if (right < h->size && h->data[i] > h->data[right]) return false;
        }
    }
    return true;
} 


// void sift_up(Heap* h, int index){//往上移動的過程 比較並交換，直到遇到比自己大(Max)或小(Min)的父節點
//     while(index > 0){
//         int parent = (index-1)/2;
//         printf("ACTION: COMPARE ( %d , %d )\n",index,parent);
//         if(compare(h, h->data[parent], h->data[index])){
            
//             printf("ACTION: SWAP ( %d , %d )\n",index,parent);
//             swap(h, &(h->data[index]), &(h->data[parent]));

//             index = parent;
//         }
//         else{
//             break;
//         }
//     }
// } 
void sift_up(Heap* h, int index){
    while(index > 0){
        int parent = (index-1)/2;
        
        // 觸發前端黃燈：正在比較
        send_state_and_block(h, "COMPARE", index, parent, false);
        
        if(compare(h, h->data[parent], h->data[index])){
            swap(h, &(h->data[index]), &(h->data[parent]));
            
            // 觸發前端紅燈並對調位置：發生交換
            send_state_and_block(h, "SWAP", index, parent, false);
            
            index = parent;
        }
        else{
            break;
        }
    }
}
// void sift_down(Heap* h, int index){//往下移動的過程 比較並交換，直到遇到比自己小(Max)或大(Min)的子節點
//     int left = 2*index+1;
//     while(left < h->size){
//         int right = 2*index+2;
//         int extreme_child = left;
//         if(right < h->size){
//             printf("ACTION: COMPARE ( %d , %d )\n",left,right);
//             if(compare(h,h->data[left],h->data[right])){
//                 extreme_child = right;
//             }
//         }

//         printf("ACTION: COMPARE ( %d , %d )\n",index,extreme_child);
//         if(compare(h,h->data[index],h->data[extreme_child])){
//             printf("ACTION: SWAP ( %d , %d )\n",index,extreme_child);
//             swap(h, &(h->data[index]), &(h->data[extreme_child]));

//             index = extreme_child;
//             left = 2*index+1;
//         }
//         else{
//             break;
//         }
//     }
// }
void sift_down(Heap* h, int index){
    int left = 2*index+1;
    while(left < h->size){
        int right = 2*index+2;
        int extreme_child = left;
        
        if(right < h->size){
            if(compare(h,h->data[left],h->data[right])){
                extreme_child = right;
            }
        }

        // 觸發前端黃燈：正在比較
        send_state_and_block(h, "COMPARE", index, extreme_child, false);
        
        if(compare(h,h->data[index],h->data[extreme_child])){
            swap(h, &(h->data[index]), &(h->data[extreme_child]));
            
            // 觸發前端紅燈並對調位置：發生交換
            send_state_and_block(h, "SWAP", index, extreme_child, false);

            index = extreme_child;
            left = 2*index+1;
        }
        else{
            break;
        }
    }
}


// void insert(Heap* h, int value){//插入在尾端 將值放在陣列尾端，size + 1，然後呼叫 sift_up 讓它浮動到正確位置
//     if(is_full(h)){
//         printf("ERROR: Heap is full!\n");
//         return;
//     }
    
//     reset_stats(h);

//     int index = h->size;
//     h->data[index] = value;
//     h->size++;
//     printf("ACTION: INSERT %d AT INDEX %d\n", value, index);

//     sift_up(h,index);
// }

void insert(Heap* h, int value){
    if(is_full(h)){
        fprintf(stderr, "ERROR: Heap is full!\n"); // 錯誤訊息改用 stderr
        return;
    }
    
    reset_stats(h);

    int index = h->size;
    h->data[index] = value;
    h->size++;
    
    // 觸發前端綠燈：節點新增在最後面
    send_state_and_block(h, "INSERTED", index, -1, false);

    sift_up(h, index);
    
    // 操作徹底結束，解除所有特效並歸還控制權
    send_state_and_block(h, "DONE", -1, -1, true);
}


// int extract_top(Heap* h){//pop 記錄頂端值準備回傳，將尾端元素移到頂端 (index 0)，size - 1，然後呼叫 sift_down 讓它沉澱
//     if(is_empty(h)){
//         printf("ERROR: Heap is empty!\n");
//         return -1;
//     }

//     reset_stats(h);

//     int top = h->data[0];
//     h->data[0] = h->data[h->size-1];
//     h->size--;
//     printf("ACTION: EXTRACT %d\n",top);
//     sift_down(h,0);
//     return top;
// }

int extract_top(Heap* h){
    if(is_empty(h)){
        fprintf(stderr, "ERROR: Heap is empty!\n"); // 錯誤訊息改用 stderr
        return -1;
    }

    reset_stats(h);

    // 告訴前端：準備把 Root 拔掉，現在先跟最後一個節點亮燈
    send_state_and_block(h, "EXTRACT_PREPARE", 0, h->size - 1, false);

    int top = h->data[0];
    h->data[0] = h->data[h->size-1];
    
    // 告訴前端：把它們兩個換位置
    send_state_and_block(h, "EXTRACT_SWAP", 0, h->size - 1, false);
    
    h->size--;
    
    if (h->size > 0) {
        // 告訴前端：尾端被砍掉了，準備開始 Sift Down
        send_state_and_block(h, "REMOVED_START_SIFT", -1, -1, false);
        sift_down(h, 0);
    }
    
    // 操作徹底結束
    send_state_and_block(h, "DONE", -1, -1, true);
    return top;
}


int peek_top(Heap* h) {
    return (h->size > 0) ? h->data[0] : -1;
}

int get_height(Heap* h) {//計算這棵樹目前有幾層
    if (h->size == 0) return 0;
    return (int)log2(h->size) + 1;
} 

int get_level(int index) {//回傳某個 index 位於樹的第幾層
    if (index < 0) return -1;
    return (int)log2(index + 1);
}

// void build_heap(Heap* h, int *arr, int n){//從雜亂的陣列建一個maxheap或是minheap
//     h->size = 0;
//     for(int i = 0; i < n && i < MAX_SIZE; i++){
//         h->data[i] = arr[i];
//         h->size++;
//     }
//     printf("ACTION: BUILD_HEAP_START\n");
//     reset_stats(h);

//     for(int i=(h->size/2)-1; i >= 0; i--){
//         printf("ACTION: BUILD_STEP AT_INDEX %d\n", i);
//         sift_down(h, i);
//     }
//     printf("ACTION: BUILD_HEAP_DONE. Total_Compare: %d\n", h->total_compare_count);
// }

void build_heap(Heap* h, int *arr, int n){
    h->size = 0;
    for(int i = 0; i < n && i < MAX_SIZE; i++){
        h->data[i] = arr[i];
        h->size++;
    }
    reset_stats(h);
    
    // 觸發前端：表示初始陣列已經載入
    send_state_and_block(h, "INSERTED", 0, h->size - 1, false);

    for(int i=(h->size/2)-1; i >= 0; i--){
        sift_down(h, i);
    }
    
    send_state_and_block(h, "DONE", -1, -1, true);
}


// void heap_sort(Heap* h){//heap sort 不斷 extract_top 將陣列由小到大或由大到小排序
//     //把雜亂的陣列變成 Heap
//     // 假設資料已經在 h->data 裡了
//     for(int i = (h->size/2)-1; i >= 0; i--){
//         sift_down(h, i);
//     }
    
//     int original_size = h->size;
//     for(int i = 0; i < original_size - 1; i++){
//         // 手動做 extract_top 的邏輯，但把彈出的值存到陣列末尾
//         int last_idx = h->size-1;
//         swap(h, &(h->data[0]), &(h->data[last_idx]));
//         h->size--;
//          printf("ACTION: HEAP_SORT CONTINUE SORTED INDEX: %d\n",last_idx);
//         sift_down(h, 0);
//     }
    
//     h->size = original_size; // 排序完後還原 size 讓前端印出完整陣列
//     printf("{\"action\": \"SORT_DONE\"}\n");
//     print_heap_state(h);
// }

void heap_sort(Heap* h){
    for(int i = (h->size/2)-1; i >= 0; i--){
        sift_down(h, i);
    }
    
    int original_size = h->size;
    for(int i = 0; i < original_size - 1; i++){
        int last_idx = h->size-1;
        
        send_state_and_block(h, "EXTRACT_PREPARE", 0, last_idx, false);
        swap(h, &(h->data[0]), &(h->data[last_idx]));
        send_state_and_block(h, "EXTRACT_SWAP", 0, last_idx, false);
        
        h->size--;
        sift_down(h, 0);
    }
    
    h->size = original_size; 
    send_state_and_block(h, "DONE", -1, -1, true);
}

// void update_key(Heap *h, int index, int new_value){// 更改某個節點的值，並重新 Sift 調整
//     if (index < 0 || index >= h->size){
//         printf("ERROR: Invalid index %d\n", index);
//         return;
//     }
//     reset_stats(h);
//     int old_value = h->data[index];
//     h->data[index] = new_value;
//     printf("ACTION: UPDATE_KEY INDEX:%d FROM:%d TO:%d\n", index, old_value, new_value);
    
//     if (index == 0) {
//         sift_down(h, 0);
//     } else {
//         int parent = (index-1)/2;
//         printf("ACTION: COMPARE_FOR_DIRECTION ( %d , %d )\n", index, parent);
//         if (compare(h, h->data[parent], h->data[index])){
//             sift_up(h, index);
//         }
//         else{
//             sift_down(h, index);
//         }
//     }
// } 
void update_key(Heap *h, int index, int new_value){
    if (index < 0 || index >= h->size){
        fprintf(stderr, "ERROR: Invalid index %d\n", index);
        return;
    }
    reset_stats(h);
    h->data[index] = new_value;
    
    // 借用 INSERTED 的特效讓修改的節點閃爍一下
    send_state_and_block(h, "INSERTED", index, -1, false);
    
    if (index == 0) {
        sift_down(h, 0);
    } else {
        int parent = (index-1)/2;
        send_state_and_block(h, "COMPARE", index, parent, false);
        if (compare(h, h->data[parent], h->data[index])){
            sift_up(h, index);
        }
        else{
            sift_down(h, index);
        }
    }
    send_state_and_block(h, "DONE", -1, -1, true);
}

// void delete_idx(Heap *h, int index){// 刪除任意 index 的節點，與尾端交換後重新 Sift 調整
//     if (index < 0 || index >= h->size){
//         printf("ERROR: Invalid index %d\n", index);
//         return;
//     }
    
//     reset_stats(h);
//     int deleted_value = h->data[index];

//     if (index == h->size-1){
//         printf("ACTION: DELETE_LAST INDEX:%d VALUE:%d\n", index, deleted_value);
//         h->size--;
//     }
//     else{
//         int last_value = h->data[h->size-1]; //將最後一個位置的人搬到index位置
//         h->size--;
//         printf("ACTION: DELETE_REPLACE INDEX:%d WITH_VALUE:%d (ORIGINAL_WAS:%d)\n", index, last_value, deleted_value);
//         update_key(h, index, last_value);
//     }
// }

void delete_idx(Heap *h, int index){
    if (index < 0 || index >= h->size){
        fprintf(stderr, "ERROR: Invalid index %d\n", index);
        return;
    }
    
    reset_stats(h);

    if (index == h->size-1){
        h->size--;
        send_state_and_block(h, "DONE", -1, -1, true);
    }
    else{
        // 借用 EXTRACT_PREPARE 的特效
        send_state_and_block(h, "EXTRACT_PREPARE", index, h->size - 1, false);
        
        int last_value = h->data[h->size-1];
        h->data[index] = last_value;
        h->size--;
        
        send_state_and_block(h, "EXTRACT_SWAP", index, h->size, false);
        
        int parent = (index-1)/2;
        if (index > 0) {
            send_state_and_block(h, "COMPARE", index, parent, false);
        }
        
        if (index > 0 && compare(h, h->data[parent], h->data[index])){
            sift_up(h, index);
        }
        else{
            sift_down(h, index);
        }
        send_state_and_block(h, "DONE", -1, -1, true);
    }
}


// void invert_heap(Heap* h) {// Max-Heap 與 Min-Heap 瞬間切換
//     printf("ACTION: INVERT_START. MODE: %s -> %s\n", h->is_max_heap ? "MAX" : "MIN", h->is_max_heap ? "MIN" : "MAX");
//     h->is_max_heap = !h->is_max_heap;
//     for (int i = (h->size / 2) - 1; i >= 0; i--) sift_down(h, i);
//     printf("ACTION: INVERT_DONE\n");
// }    

void invert_heap(Heap* h) {
    h->is_max_heap = !h->is_max_heap;
    
    // 觸發前端閃爍：準備開始翻轉
    send_state_and_block(h, "REMOVED_START_SIFT", -1, -1, false);
    for (int i = (h->size / 2) - 1; i >= 0; i--) {
        sift_down(h, i);
    }
    
    send_state_and_block(h, "DONE", -1, -1, true);
}

// void merge_heaps(Heap* h1, Heap* h2, Heap* result) {// 雙堆合併動畫
//     if (h1->size + h2->size > MAX_SIZE) { printf("ERROR: Merged size too large!\n"); return; }
//     init_heap(result, h1->is_max_heap);
//     for (int i = 0; i < h1->size; i++) result->data[result->size++] = h1->data[i];
//     for (int i = 0; i < h2->size; i++) result->data[result->size++] = h2->data[i];
//     printf("ACTION: MERGE_START. REBUILDING...\n");
//     for (int i = (result->size / 2) - 1; i >= 0; i--) sift_down(result, i);
//     printf("ACTION: MERGE_DONE\n");
// } 

void merge_heaps(Heap* h1, Heap* h2, Heap* result) {
    if (h1->size + h2->size > MAX_SIZE) { 
        fprintf(stderr, "ERROR: Merged size too large!\n"); 
        return; 
    }
    init_heap(result, h1->is_max_heap);
    for (int i = 0; i < h1->size; i++) result->data[result->size++] = h1->data[i];
    for (int i = 0; i < h2->size; i++) result->data[result->size++] = h2->data[i];
    
    // 觸發前端：合併後的初始陣列
    send_state_and_block(result, "INSERTED", 0, result->size - 1, false);
    for (int i = (result->size / 2) - 1; i >= 0; i--) {
        sift_down(result, i);
    }
    
    send_state_and_block(result, "DONE", -1, -1, true);
}

// int search_value(Heap* h, int target) {// 展現 O(N) 搜尋的動畫
//     for (int i = 0; i < h->size; i++) {
//         printf("ACTION: SEARCH_CHECK INDEX: %d VALUE: %d\n", i, h->data[i]);
//         if (h->data[i] == target) {
//             printf("ACTION: SEARCH_FOUND AT INDEX: %d\n", i);
//             return i;
//         }
//     }
//     printf("ACTION: SEARCH_NOT_FOUND\n");
//     return -1;
// }

int search_value(Heap* h, int target) {
    for (int i = 0; i < h->size; i++) {
        // 亮黃燈代表正在檢查此節點
        send_state_and_block(h, "COMPARE", i, -1, false); 
        
        if (h->data[i] == target) {
            // 找到時特殊高亮 (借用紅燈特效)
            send_state_and_block(h, "SWAP", i, -1, false); 
            send_state_and_block(h, "DONE", -1, -1, true);
            return i;
        }
    }
    send_state_and_block(h, "DONE", -1, -1, true);
    return -1;
}

int find_kth(Heap* h, int k) {// 尋找第 K 大/小元素
    if (k <= 0 || k > h->size) return -1;
    Heap temp = *h;
    int val = -1;
    for (int i = 0; i < k; i++) val = extract_top(&temp);
    return val;
}
void batch_insert(Heap* h, int *arr, int n) {// 批次匯入展示 O(N log N) 效能
    for (int i = 0; i < n; i++) insert(h, arr[i]);
}
// void clear_heap(Heap* h) {// 一鍵清空
//     h->size = 0;
//     reset_stats(h);
//     printf("ACTION: CLEAR_HEAP_SUCCESS\n");
// }

void clear_heap(Heap* h) {
    h->size = 0;
    reset_stats(h);
    send_state_and_block(h, "DONE", -1, -1, true);
}

// void _prefix(Heap* h, int i) {
//     if (i >= h->size) return;
//     printf("%d ", h->data[i]);
//     _prefix(h, 2 * i + 1);
//     _prefix(h, 2 * i + 2);
// }

void _prefix(Heap* h, int i) {
    if (i >= h->size) return;
    fprintf(stderr, "%d ", h->data[i]);
    _prefix(h, 2 * i + 1);
    _prefix(h, 2 * i + 2);
}
// void print_prefix(Heap* h) { printf("PREFIX: "); _prefix(h, 0); printf("\n"); }

void print_prefix(Heap* h) { fprintf(stderr, "PREFIX: "); _prefix(h, 0); fprintf(stderr, "\n"); }


// void _infix(Heap* h, int i) {
//     if (i >= h->size) return;
//     _infix(h, 2 * i + 1);
//     printf("%d ", h->data[i]);
//     _infix(h, 2 * i + 2);
// }

void _infix(Heap* h, int i) {
    if (i >= h->size) return;
    _infix(h, 2 * i + 1);
    fprintf(stderr, "%d ", h->data[i]);
    _infix(h, 2 * i + 2);
}

// void print_infix(Heap* h) { printf("INFIX: "); _infix(h, 0); printf("\n"); }

void print_infix(Heap* h) { fprintf(stderr, "INFIX: "); _infix(h, 0); fprintf(stderr, "\n"); }

// void _suffix(Heap* h, int i) {
//     if (i >= h->size) return;
//     _suffix(h, 2 * i + 1);
//     _suffix(h, 2 * i + 2);
//     printf("%d ", h->data[i]);
// }

void _suffix(Heap* h, int i) {
    if (i >= h->size) return;
    _suffix(h, 2 * i + 1);
    _suffix(h, 2 * i + 2);
    fprintf(stderr, "%d ", h->data[i]);
}

// void print_suffix(Heap* h) { printf("SUFFIX: "); _suffix(h, 0); printf("\n"); }

void print_suffix(Heap* h) { fprintf(stderr, "SUFFIX: "); _suffix(h, 0); fprintf(stderr, "\n"); }

int main(){
    Heap newheap;
    init_heap(&newheap, true); // 預設 Max Heap
    
    char command[100];
    while(true){
        send_state_and_block(&newheap, "IDLE", -1, -1, true);

        if(fgets(command, sizeof(command), stdin) == NULL) break;

        if(strncmp(command, "INSERT", 6) == 0){
            int value;
            if (sscanf(command, "INSERT %d", &value) == 1) insert(&newheap, value);
        }
        else if(strncmp(command, "EXTRACT", 7) == 0){
            if (!is_empty(&newheap)) extract_top(&newheap);
            else send_state_and_block(&newheap, "EMPTY", -1, -1, true);
        }
        else if(strncmp(command, "SORT", 4) == 0){
            heap_sort(&newheap);
        }
        else if(strncmp(command, "UPDATE", 6) == 0){
            int idx, val;
            if (sscanf(command, "UPDATE %d %d", &idx, &val) == 2) update_key(&newheap, idx, val);
        }
        else if(strncmp(command, "DELETE", 6) == 0){
            int idx;
            if (sscanf(command, "DELETE %d", &idx) == 1) delete_idx(&newheap, idx);
        }
        else if(strncmp(command, "INIT", 4) == 0){
            // 解析用逗號分隔的陣列字串 (例如: INIT 10,20,30)
            int arr[MAX_SIZE];
            int n = 0;
            char* p = command + 5;
            while(*p && *p != '\n') {
                int val;
                if (sscanf(p, "%d", &val) == 1) {
                    arr[n++] = val;
                    while(*p && *p != ',' && *p != '\n') p++;
                    if(*p == ',') p++;
                } else break;
            }
            build_heap(&newheap, arr, n);
        }
        else if(strncmp(command, "INVERT", 6) == 0){
            invert_heap(&newheap);
        }
        else if(strncmp(command, "CLEAR", 5) == 0){
            clear_heap(&newheap);
        }
        else if(strncmp(command, "SEARCH", 6) == 0){
            int target;
            if (sscanf(command, "SEARCH %d", &target) == 1) search_value(&newheap, target);
        }
        else if (strncmp(command, "EXIT", 4) == 0) {
            break;
        }
    }
    return 0;
}
