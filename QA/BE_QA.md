# BE QA 筆記 — spec 沒寫清楚但實作有定下來的點

讀 README_backend.md 跟 demo branch 的 `heap_logic.c` / `main.c` 之後整理。
spec 有些算式細節沒講，但實作其實已經做了選擇。
這份紀錄一下「實作目前怎麼做」，順便提醒之後 spec (README_backend.md)要補。

測試版本：`demo` branch，commit `d43c38e`（改回原本樣子 只加備份跟還原，2026-05-18）。

---

## A-1. 有重複值時，sift down 的左右選擇

例：heap = `[10, 50, 50]` sift down root，左右一樣大時要挑哪個？

**目前實作**：挑**左邊**。
看 `heap_logic.c` 的 `sift_down`：
```c
int extreme_child = left;
if (right < h->size) {
    if (compare(h, h->data[left], h->data[right])) {
        extreme_child = right;
    }
}
```
`compare` 是嚴格大於（`child > parent`），所以左右相等時不換 extreme_child，留在左邊。
最終 sift down `[10, 50, 50]` 會變成 `[50, 10, 50]`。

spec 沒寫，建議spec補一句「相等時挑左邊」。

---

## A-2. INIT 不帶值的行為

spec 寫 `INIT <values>`，沒講只送 `INIT` 會怎樣。

**目前實作**：忽略，heap 不動。
看 `main.c`：
```c
else if (strncmp(cmd, "INIT ", 5) == 0) { ... }
```
比對的是 `"INIT "`（後面有空格），所以純 `INIT\n` 不會 match 到任何分支，等於 noop，如果前端當成清空來用，會出現問題。

spec 建議補：寫「INIT 不帶值會清空 heap」，或寫「沒帶值會被忽略」。目前是後者。

---

## A-3. INSERT 滿堆時的行為

`MAX_SIZE = 100`，滿了再 INSERT 會怎樣？

**目前實作**：拒絕、寫 stderr、heap 不變。
看 `heap_insert`：
```c
if (is_full(h)) {
    fprintf(stderr, "ERROR: Heap is full!\n");
    return;
}
```
注意是寫到 stderr（符合 spec 規範一，不污染 JSON 管線）。但**沒有送任何 JSON state 給前端**，前端不會知道這次 INSERT 被拒絕。

spec 建議補 MAX_SIZE 寫進去、滿堆要送什麼事件（ERROR？）。

---

## A-4. INIT 帶多個值時，用什麼方法建 heap

`INIT 10,20,30` 兩種建法最終 heap 不一樣，spec沒寫清楚要用哪種方法，逐個 insert（top-down）或使用 heapify（bottom-up）都可以。

**目前實作**：bottom-up heapify。
`main.c` 解析完 INIT 的逗號分隔值，直接呼叫 `build_heap`：
```c
for (int i = (h->size / 2) - 1; i >= 0; i--) {
    sift_down(h, i, callback);
}
```
所以 `INIT 10,20,30` 跑出來是 `[30, 20, 10]`（bottom-up 版本），不是 `[30, 10, 20]`（逐個 insert 版本）。

spec 建議寫「INIT 多值使用 bottom-up heapify」，這樣之後 TEST_CASES 才能算 expected_heap。

---

## A-5. SORT 排出來是大到小還是小到大、排完還能不能再用

spec 第 2 節 SORT 的說明：
> 展示堆積排序。自動執行連續 Extract 動畫，排好後還原陣列。

主 README.md 雖然標題是 "heap sort visualizer"，但內文也沒解釋 SORT 的行為（只示範了 Insert 跟 Extract Max）。

讀起來不太懂幾件事：
- 「排好」是排成什麼順序？升冪還是降冪？
- 「還原陣列」是把陣列還原成SORT前的 max heap 結構？
- 排完之後使用者可以接著 INSERT / EXTRACT 等其他操作嗎？

**目前實作**：升冪（小到大），且**只還原 size，陣列已不是合法 max heap**。
看 `heap_logic.c` 的 `heap_sort`：
```c
int original_size = h->size;
for (int i = 0; i < original_size - 1; i++) {
    int last_idx = h->size - 1;
    swap(h, &(h->data[0]), &(h->data[last_idx])); // 最大值換到尾端
    h->size--;
    sift_down(h, 0, callback);
}
h->size = original_size; // 還原 size
```

每輪把 root（最大值）換到尾端、size 縮小，重複 n-1 次 → 陣列變成升冪。
最後 `size` 還原成原本大小，但陣列已經是排序狀態（不再是 max heap）。

例如 `INIT 30,10,50,20,40` 跑 SORT → `[10, 20, 30, 40, 50]`。
這時候如果再 INSERT 或 EXTRACT，行為會壞掉（前端看到的陣列不再符合 heap 性質）。

spec 建議補：
- 寫死排序方向（升 / 降）
- 還原是否是還原成SORT前的 max heap 結構
- 寫死 SORT 是不是 destructive（排完還能不能繼續用 heap）

---

## 結論

實作把 spec 沒講的地方都自己選了一個版本，功能跑得起來。但因為 spec 沒寫死，未來改實作或別人重寫都可能跑出不一樣的結果。

寫 mock_app.py 的 TEST_CASES 時會**避開相等值的測試**，因為：

- 重複值會踩到 A-1（sift down 左右選擇）的歧義
- 雖然目前實作是「挑左邊」，但 spec 沒寫死，未來改成「挑右邊」也合法

所以 mock_app.py 中的TEST_CASES 一律用**不重複的數值**，繞開 spec 模糊地帶。

---

## 怎麼跑 TEST_CASES

```bash
# 1. 編譯後端
gcc main.c heap_logic.c -o backend

# 2. 進虛擬環境
source .venv/bin/activate

# 3a. 一次跑全部
python mock_app.py --all --exe ./backend

# 3b. 或單跑某條
python mock_app.py --case 1 --exe ./backend
```

`--all` 模式最後會印通過 / 失敗總結。

目前實測 15 條全過：

```
========== 全部測試結果總結 ==========
  ✅ Case 1: INIT 多值建 heap (bottom-up)
  ✅ Case 2: INSERT 一路 sift 到 root
  ✅ Case 3: EXTRACT 連續多次
  ✅ Case 4: UPDATE 改大要 sift up
  ✅ Case 5: UPDATE 改小要 sift down
  ✅ Case 6: DELETE 中間節點
  ✅ Case 7: SORT 排序結果
  ✅ Case 8: 邊界：DELETE 最後一個 index（不需 sift）
  ✅ Case 9: 邊界：從空 heap INSERT 再 EXTRACT 變回空
  ✅ Case 10: 混合：INIT + INSERT + EXTRACT + UPDATE
  ✅ Case 11: 邊界：INIT 只放一個值
  ✅ Case 12: 邊界：DELETE root 應等同 EXTRACT
  ✅ Case 13: 混合：連續 INSERT 多次後連續 EXTRACT
  ✅ Case 14: 混合：INIT 後兩個不同方向的 UPDATE
  ✅ Case 15: 混合：SORT 後再INSERT

共 15 條，通過 15，失敗 0
```

---

# B. 後續實作的修改

QA 過程中順手改的後端 code，順帶解掉前面 A-5 跟一些動畫問題。

## B-1. 重寫 heap_sort 成非 destructive

原本 heap_sort 跑完之後陣列變成升冪、不再是合法 max heap，使用者再 INSERT/EXTRACT 行為會壞掉（對應 A-5 提到的問題）。

新版做的事：
- 開頭備份 `original_data[]`
- 主迴圈不縮 `h->size`，改用 local `boundary` 變數標 heap 區大小（這樣前端能一直看到完整陣列，max 換到尾端後不會消失）
- 迴圈內每輪：swap root 跟尾端 → boundary-- → sift_down 新 root
- 跑完送 DONE（升冪畫面，給使用者看一下）
- 從 backup 還原回原本 heap → 再送 DONE
- 拿掉開頭多餘的 heapify 迴圈（呼叫前 heap 已經合法，不需要再 sift_down 一輪）
- 拿掉 swap 前的假 COMPARE 事件（標準 heap sort 在 swap 前不 compare）

## B-2. 新增 sort_boundary 欄位讓前端可以畫已排序區

為了讓前端把已排序的元素標灰：
- `heap_logic.h` 的 `VisualState` struct 加 `int sort_boundary` 欄位（-1 表示沒已排序區）
- `heap_logic.c` 加 `trigger_state_with_bound` helper，給 heap_sort 用；既有的 `trigger_state` 預設 sort_boundary=-1
- `main.c` 的 `send_state_and_block` JSON 多送 `"sort_boundary": <int>`
- 順手修了 `main.c` 第 50、64 行的 IDLE / EMPTY 狀態 — 加上新欄位之後，positional initializer 漏給最後一個值會被 zero-init 成 0，導致前端把整個 heap 都當已排序區畫灰。明確補 `, -1` 修掉。

