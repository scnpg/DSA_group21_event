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
    int swap_count; //交換次數
    int compare_count; //比較次數
    bool is_max_heap;
}Heap;

typedef struct Info{
    int parent;
    int self;
}info;

void swap(Heap* h, int* a, int* b){
    int temp = *a;
    *a = *b;
    *b = temp;
    h->swap_count++;
}

bool compare(Heap* h, int parent_val, int child_val){
    h->compare_count++;
    if(h->is_max_heap){
        return child_val > parent_val;
    }
    else{
        return child_val < parent_val;
    }
}

//todo
void init_heap(Heap* h, bool is_max); // 初始化 size = 0 與設定 max/min 模式
void reset_stats(Heap* h);            // 歸零統計數據，每次操作前呼叫
void print_heap_state(Heap* h);       // 把當前陣列狀態印出來給 Python 讀取

bool is_empty(Heap* h); // 前端可以用來決定是否要把 Extract 按鈕反灰
bool is_full(Heap* h); // 前端可以用來決定是否要把 Insert 按鈕反灰
bool is_valid_heap(Heap* h); // 檢查當前陣列是否真的符合 Heap 規則 (除錯用)

void sift_up(Heap* h, int index); //往上移動的過程 比較並交換，直到遇到比自己大(Max)或小(Min)的父節點
void sift_down(Heap* h, int index);//往下移動的過程 比較並交換，直到遇到比自己小(Max)或大(Min)的子節點

info trace(Heap* h, int index);//將每一步和誰swap，跟和誰compare都回傳

void insert(Heap* h, int value);//插入在尾端 將值放在陣列尾端，size + 1，然後呼叫 sift_up 讓它浮動到正確位置
int extract_top(Heap* h);//pop 記錄頂端值準備回傳，將尾端元素移到頂端 (index 0)，size - 1，然後呼叫 sift_down 讓它沉澱
int peek_top(Heap* h);
int get_height(Heap* h); //計算這棵樹目前有幾層
int get_level(int index);//回傳某個 index 位於樹的第幾層

void build_heap(Heap* h, int *arr, int n);//從雜亂的陣列建一個maxheap或是minheap
void heap_sort(Heap* h);//heap sort 不斷 extract_top 將陣列由小到大或由大到小排序

void update_key(Heap *h, int index, int new_value); // 更改某個節點的值，並重新 Sift 調整
void delete_idx(Heap *h, int index);                // 刪除任意 index 的節點，與尾端交換後重新 Sift 調整

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