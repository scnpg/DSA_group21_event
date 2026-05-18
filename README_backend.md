# 🧠 Heap Visualizer IPC 通訊協定與系統架構

本專案採用 **MVC 架構與微服務 (Microservices)** 精神設計，將「視覺化前端 (Python UI)」與「演算法引擎 (C Backend)」完全解耦。雙方透過作業系統標準管線 (Standard I/O Pipes) 進行非同步的阻塞式通訊 (Blocking IPC)。

---

## 1. 核心底層邏輯：Heap 陣列存儲規則

雙方皆須嚴格遵守以下以 $0$ 為起點 (0-based) 的一維陣列扁平化儲存規則，以確保前端能將 C 語言的一維陣列正確繪製成二元樹結構：

* **根節點 (Root)**：永遠位於索引 $0$。
* **父子節點計算**：給定節點索引 $i$，其父節點為 $\lfloor (i - 1) / 2 \rfloor$，左子節點為 $2i + 1$，右子節點為 $2i + 2$。

---

## 2. 輸入指令集 (Input Commands : Python ➜ C)

前端寫入的大指令必須以「空格」分隔參數，以「逗號」分隔陣列，**且結尾必須附帶 `\n` (換行符號) 並執行 Flush**。

| 指令格式 | 系統行為說明 |
| --- | --- |
| **`INIT <values>`** | **系統重置與建樹。** 收到此指令代表「砍掉重練」。後端會清空當前狀態，並在底層**瞬間算完**建樹演算法（此過程不觸發任何中介動畫），計算完畢後直接回到主迴圈輸出 `IDLE` 狀態。 |
| **`INSERT <value>`** | **新增節點。** 將數值放於陣列最尾端，並執行 Sift Up。 |
| **`EXTRACT`** | **提取頂端節點。** 取出 Root 節點，將尾端節點遞補至 Root，並執行 Sift Down。 |
| **`UPDATE <idx> <val>`** | **更新數值。** 強制更改指定位置的值，並自動判斷執行 Sift Up/Down。 |
| **`DELETE <idx>`** | **刪除節點。** 移除指定位置節點，尾端遞補，並重新調整。 |
| **`SORT`** | **展示堆積排序。** 自動執行連續 Extract 動畫，排好後還原陣列。 |
| **`\n`** (純換行) | **步進解鎖 (Step Forward)。** 前端動畫影格播放完畢，使用者點擊下一步時送出，用以解除 C 語言的 `fgets` 阻塞，推動演算法進行。 |

---

## 3. C Backend 開發規範與防呆機制

為確保 IPC 管線穩定，負責 C 語言的開發者請嚴格遵守以下 3 大作業系統層級規範：

### ⚠️ 規範一：Debug 訊息嚴禁使用 `stdout`

在 IPC 架構下，`stdout` (標準輸出) 是專屬 JSON 資料的傳輸管線。**若使用 `printf` 印出非 JSON 的除錯字串，Python 端的解析器會立刻崩潰！**

* **正確寫法**：請一律使用 **`fprintf(stderr, "除錯訊息\n");`**。寫入標準錯誤 (Standard Error) 的訊息會直接顯示在終端機，完全不會污染 JSON 管線。

### ⚠️ 規範二：管線斷裂與生命週期 (EOF Handling)

若 Python 視窗被使用者關閉，連接雙方的 Pipe 會斷裂 (Broken Pipe)。

* **正確寫法**：主迴圈監聽指令時，**只要 `fgets` 讀到 `NULL` (EOF)，或讀到 `EXIT` 字串，C 語言進程必須立刻 `return 0` 或 `exit(0)` 關閉自己**，避免成為耗損系統資源的背景殭屍行程。

### ⚠️ 規範三：統一使用 `fgets` 取代 `scanf`

`scanf` 容易在緩衝區殘留換行符號，導致後續的步進阻塞被異常跳過。主迴圈請統一使用 `fgets` 讀取一整行字串，再搭配 `strtok` 或 `sscanf` 來解析參數。

---

## 4. 輸出狀態集與觸發時機 (State Triggers)

每一次 C 語言狀態改變，都必須輸出一個 JSON 影格。為了保證前後端高度同步，請**嚴格在指定的程式碼位置**呼叫 `arrange_and_print_state`。

### 4.1 `DONE` 與 `IDLE` 的設計差異 (重要)

這兩個狀態負責控制系統的節奏，它們的意義與阻塞邏輯完全不同：

* **`DONE` (演算法單次操作結束)**：
* **意義**：通知前端「這次的操作（如 Sift）到底了」。前端收到後，會將所有節點的顏色重置回預設狀態（清除 COMPARE 或 SWAP 留下的亮燈）。
* **狀態**：`is_idle = false`。C 語言吐出 `DONE` 後依然會阻塞，等待使用者最後按一次「下一步」來欣賞最終結果。


* **`IDLE` (系統全域閒置)**：
* **意義**：通知前端「C 語言已經回到主迴圈，準備好接客了」。前端收到後，會解鎖所有輸入框與操作按鈕（如 Insert, Extract）。
* **狀態**：`is_idle = true`。此時 `arrange_and_print_state` **不會阻塞**。C 語言吐出後會立刻進入 `main` 的 `fgets` 阻塞，等待前端下達全新的大指令。



### 4.2 事件字典與確切呼叫時機

請在以下 7 個時機點的「下一行」送出對應狀態：

1. **`INSERTED`** (新增於尾端)
* **時機**：在 `insert` 函式中，數值寫入 `data[size]` 且 `size++` 完成後。
* **前端行為**：於陣列尾端生成新節點，亮綠燈。


2. **`EXTRACTED`** (拔除並遞補)
* **時機**：在 `extract` 函式中，將 `data[size-1]` 移交給 `data[0]` 且 `size--` 完成後。
* **前端行為**：移除 Root，將最後一個節點移至頂端，亮藍燈。


3. **`UPDATED`** (直接修改數值)
* **時機**：在 `update_key` 函式中，將 `data[index] = new_value` 賦值完成後。
* **前端行為**：更新該節點內的數字顯示。


4. **`COMPARE`** (比較節點)
* **時機**：在 Sift 迴圈中，**準備執行 `if (data[parent] < data[child])` 的上一行**。
* **前端行為**：將目標節點閃爍黃燈。


5. **`SWAP`** (記憶體交換)
* **時機**：決定進入 `if` 條件後，**陣列數值交換完成的下一行**。
* **前端行為**：將目標節點轉紅燈，並播放位置對調動畫。


6. **`DONE`** (中介演算法結束)
* **時機**：Sift 迴圈因符合規定而 `break`，或是跑到極限位置時，即**整個 `insert`, `extract`, `update_key` 函式的最後一行**。
* **前端行為**：重置所有節點顏色至預設狀態。


7. **`IDLE`** (回到主迴圈)
* **時機**：在 `main` 函式中，剛啟動時，或是執行完一次指令的函式並**回到 `while` 迴圈開頭時**。
* **前端行為**：解鎖全域操作面板。



---

## 5. 狀態通訊介面實作

請在 C 程式碼中定義 `IPCState`，並使用 `arrange_and_print_state` 進行通訊同步。此函式會負責：**整理 JSON、強制推播 (Flush)、並視情況自動阻塞等待前端的 `\n`**。

```c
#include <stdio.h>
#include <stdbool.h>

// 定義一個動畫影格的狀態資訊
typedef struct {
    const char* event;       // "SWAP", "COMPARE", "IDLE" 等字串
    int target1;             // 目標索引 1 (-1 代表無)
    int target2;             // 目標索引 2 (-1 代表無)
    bool is_idle;            // 是否為閒置狀態 (控制是否需要步進阻塞)
} IPCState;

// 將 IPCState 序列化為 JSON，並執行管線同步
// (註：參數 Heap* h 請依實際的 struct 定義替換)
void arrange_and_print_state(Heap* h, IPCState state) {
    // 1. Arrange JSON: 陣列狀態
    printf("{\"heap\": [");
    for(int i = 0; i < h->size; i++) {
        printf("%d%s", h->data[i], (i == h->size - 1) ? "" : ", ");
    }
    
    // 2. Arrange JSON: 事件與目標陣列
    printf("], \"size\": %d, \"event\": \"%s\", \"targets\": ", h->size, state.event);
    if (state.target1 != -1 && state.target2 != -1) {
        printf("[%d, %d]", state.target1, state.target2);
    } else if (state.target1 != -1) {
        printf("[%d]", state.target1);
    } else {
        printf("[]");
    }

    // 3. Arrange JSON: 閒置旗標與儀表板數據
    printf(", \"is_idle\": %s, ", state.is_idle ? "true" : "false");
    printf("\"stats\": {\"cur_swap\": %d, \"cur_cmp\": %d, \"tot_swap\": %d, \"tot_cmp\": %d}}\n",
           h->cur_swap_count, h->cur_compare_count, h->total_swap_count, h->total_compare_count);
           
    // 4. Print & Flush: 寫入管線並強制推播，確保 Python 立即收到
    fflush(stdout); 
    
    // 5. IPC Blocking: 若為動畫影格(is_idle = false)，主動交出 CPU，等待前端按下下一步
    if (!state.is_idle) {
        char step_buffer[10];
        fgets(step_buffer, sizeof(step_buffer), stdin);
    }
}

```

---

## 6. 後端合約測試 (Contract Testing) —— `mock_app.py`

為確保 C Backend 能夠獨立開發且不依賴視覺化前端的進度，Python 團隊提供了 `mock_app.py` 測試腳本。

該腳本承諾了未來正式 Python UI 的「非同步讀取」與「步進解鎖」邏輯。腳本會扮演一個點擊速度極快的使用者：它只要讀取到 JSON 中的 `is_idle: false`，就會自動送出 `\n` 來解鎖 C 語言。

### 開發與驗證流程：

1. **設定測資**：打開 `mock_app.py`，在頂部的 `TEST_CASES` 字典中新增或修改極端測試案例 (Edge Cases)。
2. **編譯後端**：編譯您的 C 程式碼（例如：`gcc main.c -o main`）。
3. **執行測試**：透過命令列啟動測試腳本，指定要跑的 Case 編號與執行檔路徑：
```bash
python mock_app.py --case 1 --exe ./main

```


4. **驗證結果**：若終端機輸出 `✅ Pass!`，代表您的演算法邏輯與 IPC 通訊協定皆已達標，可隨時與前端進行無縫整合。

```
python mock_app.py --case 1 --exe ./main
```

---

## 7. Spec 補充說明（邊界行為定義）

以下為 QA 過程中發現的 spec 模糊點，現補充為正式規範。

### 7-1. Sift Down 遇到左右子節點相等時的選擇

**規範**：左右子節點數值相等時，**選左子節點**繼續 sift down。

實作依據：`sift_down` 內部使用嚴格大於（`>`）做比較，相等時不更換極值子節點，故預設停留在左邊。

範例：heap = `[10, 50, 50]` → sift down root → `[50, 10, 50]`（左子升頂）。

---

### 7-2. `INIT` 不帶值時的行為

**規範**：`INIT` 後不附帶任何值（即純 `INIT\n`）時，後端**忽略此指令，heap 不變**。

後端採用 `strncmp(cmd, "INIT ", 5)` 比對帶空格的前綴，純 `INIT` 不符合任何分支，等同 noop。若要清空 heap，請改用 `CLEAR` 指令。

---

### 7-3. `INSERT` 滿堆時的行為

**規範**：`MAX_SIZE = 100`（定義於 `heap_logic.h`）。堆滿時再 `INSERT`，後端會：

1. 拒絕插入，heap 不變。
2. 以 `fprintf(stderr, ...)` 寫入錯誤訊息（不污染 JSON 管線）。
3. **不送任何 JSON 狀態給前端**，前端畫面維持不動。

前端不會收到 INSERT 被拒絕的通知，展示時請避免超過 100 個節點。

---

### 7-4. `INIT` 帶多個值時的建堆方法

**規範**：`INIT v1,v2,v3,...` 使用 **bottom-up heapify（Floyd 演算法）** 建堆，時間複雜度 O(n)。

流程：先將所有值依序填入陣列，再從 `size/2 - 1` 到 `0` 依序執行 `sift_down`。

與逐個 INSERT（top-down）結果不同：`INIT 10,20,30` → `[30, 20, 10]`（bottom-up），而非 `[30, 10, 20]`（top-down）。mock_app.py 的 expected_heap 均依此計算。

---

### 7-5. `SORT` 排序方向與是否 destructive

**規範**：

* **排序方向**：升冪（小 → 大）。heap sort 每輪把最大值換到尾端，最終陣列由小到大。
* **非 destructive**：SORT 完成後後端會**自動還原**成 SORT 前的 max heap 狀態（備份並恢復 `data[]`），使用者可繼續執行 INSERT / EXTRACT 等操作。
* **動畫流程**：排序完成後先送一次 DONE（讓使用者看到升冪結果），再還原並送 DONE（回到原 heap），最後送 IDLE 解鎖操作面板。