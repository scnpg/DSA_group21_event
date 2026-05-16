# DSA Project: Max Heap Visualizer.

## Description
A visualizer to understand how heap sort works. Nodes of interest will be highlighted, and each step of heapify is displayed for the insertion and removal of nodes.
## How It Works

### Max Heap Structure

A max heap is a binary tree where every parent node is greater than or equal to its children.

The heap is stored as an array:
- Parent index: i
- Left child: 2i + 1
- Right child: 2i + 2

Example:
```
[50, 40, 30, 20]

        50
       /  \
     40    30
    /
  20
```

---

### Core Operations

#### 1. Insertion (Sift Up)

1. Insert the new element at the end of the array
2. Compare it with its parent
3. Swap if the new element is larger
4. Repeat until the heap property is restored

---

#### 2. Removal (Extract Max)

1. Swap the root with the last element
2. Remove the last element
3. Perform sift down from the root:
   - Compare with children
   - Swap with the largest child
   - Continue until the heap property is restored

---

#### 3. Heapify (Sift Down)

This is the key operation used in both removal and heap sort:
- Compare a node with its children
- Swap with the largest child if necessary
- Repeat recursively

---

### Visualization Design

This project separates computation and visualization:

- C backend: performs heap operations
- Python (Flet) frontend: renders the heap and UI

The backend does not directly control the UI. Instead, it emits intermediate states as JSON.

Each step of the algorithm produces a state:

```json
{
  "heap": [40, 20, 30],
  "event": "SWAP",
  "targets": [0, 1],
  "is_idle": false
}
```

---

### Step-by-Step Execution

The system uses a blocking mechanism to control execution:

1. The C backend outputs a state (JSON)
2. The Python frontend renders it
3. The backend pauses and waits for input
4. When the user clicks "Step", Python sends a signal
5. The backend continues to the next step

This allows users to observe the algorithm one operation at a time.


## Dependencies
```bash
pip -r install requirements.txt
```
（你可能需要建立 python 虛擬環境）


## 運行 app

*   主 APP（demo python 跟 C 的交互）
```bash
flet run app.py
```

*   heap demo
```bash
flet run heap_demo.py
```

## What to do

1. Insertion: enter a value in the text box, and click insert to see your value be added to the heap. Heapify after.
2. Removal: Remove the top node and heapify.
3. Clear: Gives you an empty heap. 

Click step to continue with sort. The computer's operations will be displayed at the top of the window. 

## Example: Removing the Maximum Element (Extract Max)

We demonstrate how the heap updates step-by-step when removing the root (maximum value).

### Initial Heap

Array representation:
```
[50, 40, 30, 20]
```

Tree representation:
```
        50
       /  \
     40    30
    /
  20
```

---

### Step 1: Prepare for Extraction

The root and last element are selected for swapping.

```json
{
  "heap": [50, 40, 30, 20],
  "event": "EXTRACT_PREPARE",
  "targets": [0, 3],
  "is_idle": false
}
```

---

### Step 2: Swap Root with Last Element

The root (50) is swapped with the last element (20).

Array after swap:
```
[20, 40, 30, 50]
```

```json
{
  "heap": [20, 40, 30, 50],
  "event": "EXTRACT_SWAP",
  "targets": [0, 3],
  "is_idle": false
}
```

---

### Step 3: Remove Last Element

The last element (50) is removed from the heap.

New heap:
```
[20, 40, 30]
```

---

### Step 4: Sift Down (Heapify)

Compare the root with its children:

```json
{
  "heap": [20, 40, 30],
  "event": "COMPARE",
  "targets": [0, 1, 2],
  "is_idle": false
}
```

Swap with the largest child (40):

Array after swap:
```
[40, 20, 30]
```

```json
{
  "heap": [40, 20, 30],
  "event": "SWAP",
  "targets": [0, 1],
  "is_idle": false
}
```

---

### Step 5: Done

The heap property is restored.

```json
{
  "heap": [40, 20, 30],
  "event": "DONE",
  "targets": [],
  "is_idle": true
}
```

---

### Final Heap

```
[40, 20, 30]
```

Tree representation:
```
        40
       /  \
     20    30
```

---

### Summary

- The root is swapped with the last element.
- The last element is removed.
- The heap is restored using sift down.
- Each step is emitted as a JSON state for visualization.

