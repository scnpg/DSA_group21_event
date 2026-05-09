#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap_logic.h"

int main() {
    // 關閉緩衝區，確保 IPC 通訊流暢
    setvbuf(stdout, NULL, _IONBF, 0);
    
    Heap h;
    init_heap(&h, true); // 預設以 Max-Heap 啟動
    
    char command[50];
    int value;
    
    // 進入無窮迴圈，持續監聽來自 Python app.py 的 stdin 輸入
    while(scanf("%s", command) != EOF) {
        if(strcmp(command, "INSERT") == 0) {
            if(scanf("%d", &value) == 1) {
                insert(&h, value);
            }
        } 
        else if(strcmp(command, "EXTRACT") == 0) {
            extract_top(&h);
        }
        else if(strcmp(command, "INIT") == 0) {
            // 前端要求重置時可呼叫 INIT
            init_heap(&h, true);
        }
        // 注意：若是前端的 "下一步" 按鈕，Python 只會傳送 \n 空白行
        // 這會被 heap_logic.c 裡面的 fgets 吃掉以解鎖動畫，所以這裡不用特別處理空字串
    }
    
    return 0;
}