import flet as ft
import flet.canvas as cv
import math


# ── 狀態文字 ──────────────────────────────────────────────────────
def get_status_text(state):
    if state.is_idle:
        return "Status: Ready (Idle)"

    e = state.event
    t = state.targets

    if e == "INSERTED":
        return f"Action: 新節點插入位置 {t}，開始 Sift Up..."

    elif e == "EXTRACT_PREPARE":
        return f"Action: 準備移除 Root，標記 Root {t} 與最後節點..."

    elif e == "EXTRACT_SWAP":
        return f"Action: 將 Root 與最後節點 {t} 對調..."

    elif e == "REMOVED_START_SIFT":
        return "Action: Root 已移除，開始 Sift Down 恢復 Heap 性質..."

    elif e == "DELETE_PREPARE":
        idx = t[0] if t else "?"
        return f"Action: 準備刪除節點 [{idx}]，將其與最後節點對調..."

    elif e == "DELETE_SWAP":
        return f"Action: 節點已移至末端並移除，調整 Heap 結構中..."

    elif e == "INVERT_START":
        return "Action: 切換 Heap 類型，開始重新 Heapify（bottom-up Sift Down）..."

    elif e == "COMPARE":
        return f"Action: 比較節點 {t}..."

    elif e == "SEARCH_CMP":
        cur = t[0] if t else "?"
        found_so_far = t[1:] if len(t) > 1 else []
        hint = f"　已找到：{found_so_far}" if found_so_far else ""
        return f"Action: 搜尋中，檢查節點 [{cur}]...{hint}"

    elif e == "FOUND":
        return f"Action: ✅ 找到目標！節點 {t}（共 {len(t)} 個），繼續搜尋..."

    elif e == "SEARCH_DONE":
        return f"Action: 搜尋完成！共找到 {len(t)} 個目標節點：{t}"

    elif e == "SEARCH_NOT_FOUND":
        return "Action: 搜尋完成，未找到目標值。"

    elif e == "SWAP":
        if state.sort_boundary != -1:
            return f"Action: [Heap Sort] 交換節點 {t}，排序邊界 = {state.sort_boundary}..."
        return f"Action: 交換節點 {t}..."

    elif e == "DONE":
        if state.sort_boundary == 0:
            return "Action: Heap Sort 完成！陣列已排序（升冪）。"
        return "Action: 操作完成！"

    elif e == "EMPTY":
        return "Status: Heap 已空，無法執行此操作。"

    elif e == "IPC_ERROR":
        return "⚠️ IPC 解析錯誤，請重啟程式。"

    elif e == "BACKEND_DIED":
        return "⚠️ 後端程序已結束，請重啟程式。"

    return f"Action: {e} 進行中..."


# ── 顏色工具 ──────────────────────────────────────────────────────
def _is_sorted(i, state):
    return state.sort_boundary != -1 and i >= state.sort_boundary


def _array_bgcolor(i, state):
    if _is_sorted(i, state):
        return ft.Colors.GREY_400

    e = state.event
    if e == "SEARCH_CMP":
        if state.targets and i == state.targets[0]:   # 當前比較節點：橘
            return ft.Colors.AMBER_200
        if i in state.targets[1:]:                    # 已找到的節點：綠
            return ft.Colors.GREEN_200
    elif e in ("FOUND", "SEARCH_DONE"):
        if i in state.targets:
            return ft.Colors.GREEN_200
    elif e == "SEARCH_NOT_FOUND":
        return ft.Colors.BLUE_50
    elif i in state.targets:
        return ft.Colors.AMBER_200

    return ft.Colors.BLUE_50


def _tree_bgcolor(i, state):
    if _is_sorted(i, state):
        return ft.Colors.GREY_600

    e = state.event
    if e == "SEARCH_CMP":
        if state.targets and i == state.targets[0]:   # 當前比較節點：橘
            return ft.Colors.ORANGE_800
        if i in state.targets[1:]:                    # 已找到的節點：綠
            return ft.Colors.GREEN_700
    elif e in ("FOUND", "SEARCH_DONE"):
        if i in state.targets:
            return ft.Colors.GREEN_700
    elif i in state.targets:
        return ft.Colors.ORANGE_800

    return ft.Colors.BLUE_700


# ── 主要渲染函式 ──────────────────────────────────────────────────
def create_heap_view(state, mode="tree"):

    # ── Array View ────────────────────────────────────────────────
    if mode == "array":
        if not state.heap:
            return ft.Container(
                content=ft.Text("No Data", color=ft.Colors.GREY_500),
                alignment=ft.alignment.center,
                padding=20,
            )

        cells = []
        for i, val in enumerate(state.heap):
            cells.append(
                ft.Container(
                    content=ft.Column([
                        ft.Text(str(val),  size=12, weight="bold"),
                        ft.Text(f"[{i}]",  size=9,  color=ft.Colors.GREY_600),
                    ], alignment=ft.MainAxisAlignment.CENTER,
                       horizontal_alignment=ft.CrossAxisAlignment.CENTER,
                       spacing=0),
                    width=44, height=44,
                    bgcolor=_array_bgcolor(i, state),
                    border=ft.border.all(1, ft.Colors.BLUE_400),
                    border_radius=4,
                    alignment=ft.alignment.center,
                )
            )

        return ft.Container(
            content=ft.Row(
                controls=cells,
                wrap=True,
                spacing=4,
                run_spacing=4,
                width=460,
            ),
            padding=10,
            alignment=ft.alignment.top_center,
        )

    # ── Tree View ─────────────────────────────────────────────────
    else:
        if not state.heap:
            return ft.Container(
                content=ft.Text("No Data", color=ft.Colors.GREY_500),
                alignment=ft.alignment.center,
                padding=20,
            )

        n          = len(state.heap)
        max_depth  = int(math.log2(n)) if n > 1 else 0
        bottom_cnt = 2 ** max_depth          # 最底層節點數

        # ── 動態計算尺寸，確保節點不超出邊界 ──────────────────────
        canvas_w = 680

        # node_r：以底層節點能完整排下為準，最小 10px 最大 18px
        spacing  = canvas_w / (bottom_cnt + 1)   # 底層相鄰節點圓心間距
        node_r   = min(18.0, max(10.0, spacing / 2 - 2))

        # level_h：垂直層間距，隨深度縮小避免超出畫布高度
        level_h  = min(72, max(48, int(340 / max(max_depth, 1))))

        top_pad  = 28
        # 依實際深度決定畫布高度（最低 380px）
        canvas_h = max(380, max_depth * level_h + top_pad + int(node_r * 2) + 12)

        # ── 計算每個節點的圓心座標 ────────────────────────────────
        centers = []
        for i in range(n):
            depth           = int(math.log2(i + 1))
            pos_in_level    = i - (2 ** depth - 1)
            total_in_level  = 2 ** depth
            cx = (canvas_w / (total_in_level + 1)) * (pos_in_level + 1)
            cy = depth * level_h + top_pad
            centers.append((cx, cy))

        # Canvas：父子連線（底層）
        lines = [
            cv.Line(
                x1=centers[(i - 1) // 2][0],
                y1=centers[(i - 1) // 2][1],
                x2=centers[i][0],
                y2=centers[i][1],
                paint=ft.Paint(
                    color=ft.Colors.GREY_400,
                    stroke_width=2,
                    style=ft.PaintingStyle.STROKE,
                ),
            )
            for i in range(1, len(centers))
        ]
        line_canvas = cv.Canvas(lines, width=canvas_w, height=canvas_h)

        # 節點圓圈（疊在 Canvas 上層）
        font_size = max(8, int(node_r * 0.7))   # 字體隨節點等比縮放
        nodes = [
            ft.Container(
                content=ft.Text(str(val), color="white", weight="bold",
                                size=font_size, no_wrap=True),
                bgcolor=_tree_bgcolor(i, state),
                width=int(node_r * 2),
                height=int(node_r * 2),
                shape=ft.BoxShape.CIRCLE,
                alignment=ft.alignment.center,
                left=cx - node_r,
                top=cy  - node_r,
            )
            for i, (val, (cx, cy)) in enumerate(zip(state.heap, centers))
        ]

        # 用 Container + clip 包住 Stack，避免節點溢出邊框
        return ft.Container(
            content=ft.Stack([line_canvas] + nodes,
                             width=canvas_w, height=canvas_h),
            width=canvas_w,
            height=canvas_h,
            clip_behavior=ft.ClipBehavior.HARD_EDGE,
        )